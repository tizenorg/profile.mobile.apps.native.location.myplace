/*
 *  myplace
 *
 * Copyright (c) 2000 - 2011 Samsung Electronics Co., Ltd. All rights reserved.
 *
 * Contact: Jongmun Woo <jongmun.woo@samsung.com>, Kyoungjun Sung <kj7.sung@samsung.com>, Young-Ae Kang <youngae.kang@samsung.com>
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 * http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 *
 */


#include <app.h>
#include <ui-gadget.h>
#include <vconf.h>
#include <Elementary.h>
#include <libintl.h>
#include <locations.h>
#include <dlog.h>
#include <locations.h>

#include "myplace-common.h"
#include "myplace-mapview.h"
#include "myplace-elementary.h"
#include "myplace-detailinfo.h"

static bool service_enabled = false;
static GThread *thread = NULL;
static location_manager_h manager;

static void __state_changed_cb(location_service_state_e state, void *user_data)
{
	switch (state) {
	case LOCATIONS_SERVICE_ENABLED:
		service_enabled = true;
		break;
	case LOCATIONS_SERVICE_DISABLED:
		service_enabled = false;
		break;
	default:
		break;
	}
}

static void addressOnMapsService_cb(maps_error_e result, int request_id, int index, int total, maps_address_h address, void *user_data)
{
	myplace_app_data *ad = (myplace_app_data *)user_data;
	char *result_address = NULL;
	int ret = 0;

	if (ad->mapview_place->address != NULL) {
		free(ad->mapview_place->address);
		ad->mapview_place->address = NULL;
	}

	ret = maps_address_get_freetext(address, &result_address);
	if (ret != MAPS_ERROR_NONE) {
		LS_LOGE("maps_address_get_freetext fail, error = %d", ret);
		result_address = NULL;
	}
	if (result_address == NULL) {
		char address[30] = {};
		snprintf(address, sizeof(address), "%lf, %lf", ad->mapview_place->latitude, ad->mapview_place->longitude);
		ad->mapview_place->address = strdup(address);
	} else
		ad->mapview_place->address = strdup(result_address);

	if (result_address != NULL) {
		free(result_address);
		result_address = NULL;
	}

	elm_entry_entry_set(ad->map_entry, ad->mapview_place->address);
	elm_object_disabled_set(ad->map_done_btn, EINA_FALSE);
}

static void showPositionOnMapsService(myplace_app_data *ad)
{
	maps_coordinates_h maps_coord = NULL;
	maps_view_object_h marker = NULL;
	int ret = 0;

	ret = maps_coordinates_create(ad->mapview_place->latitude, ad->mapview_place->longitude, &maps_coord);
	if (ret != MAPS_ERROR_NONE) {
		LS_LOGE("maps_coordinates_create fail, error = %d", ret);
		return;
	}
	maps_view_set_zoom_level(ad->maps_view, 12);

	if (ad->mapview_place->address != NULL)
		elm_entry_entry_set(ad->map_entry, ad->mapview_place->address);

	ret = maps_view_set_center(ad->maps_view, maps_coord);
	if (ret != MAPS_ERROR_NONE) {
		LS_LOGE("maps_view_set_center fail, error = %d", ret);
		return;
	}
	ret = maps_view_remove_all_objects(ad->maps_view);
	if (ret != MAPS_ERROR_NONE) {
		LS_LOGE("maps_view_remove_all_objects fail, error = %d", ret);
		return;
	}
	ret = maps_view_object_create_marker(maps_coord, IMG_DIR"/marker_icon.png", MAPS_VIEW_MARKER_PIN, &marker);
	if (ret != MAPS_ERROR_NONE) {
		LS_LOGE("map_object_create_marker fail, error = %d", ret);
		return;
	}

	ret = maps_view_add_object(ad->maps_view, marker);
	if (ret != MAPS_ERROR_NONE) {
		LS_LOGE("maps_view_add_object fail, error = %d", ret);
		return;
	}

	if (maps_coord != NULL)
		maps_coordinates_destroy(maps_coord);
}


static void position_updated(double latitude, double longitude, double altitude, time_t timestamp, void *user_data)
{
	myplace_app_data *ad = (myplace_app_data *)user_data;
	maps_preference_h preference = NULL;
	int ret = 0;
	int request_id = 0;

	if (service_enabled) {
		ad->mapview_place->latitude = latitude;
		ad->mapview_place->longitude = longitude;

		ret = maps_preference_create(&preference);
		if (ret != MAPS_ERROR_NONE) {
			LS_LOGE("maps_preference_create fail, error = %d", ret);
			return;
		}

		ret = maps_service_reverse_geocode(ad->maps_service, ad->mapview_place->latitude, ad->mapview_place->longitude, preference, addressOnMapsService_cb, ad, &request_id);
		if (ret != MAPS_ERROR_NONE) {
			LS_LOGE("maps_service_reverse_geocode fail, error = %d", ret);
			return;
		}

		showPositionOnMapsService(ad);

		ret = maps_preference_destroy(preference);
		if (ret != MAPS_ERROR_NONE) {
			LS_LOGE("maps_preference_destroy fail, error = %d", ret);
			return;
		}
	}
}

static gpointer wait_for_service(void *data)
{
	int timeout = 0;

	for (; timeout < 30; timeout++) {
		if (service_enabled)
			break;
		else
			sleep(1);
	}

	if (manager != NULL) {
		location_manager_stop(manager);
		location_manager_unset_position_updated_cb(manager);
		location_manager_unset_service_state_changed_cb(manager);
		location_manager_destroy(manager);
		manager = NULL;
		thread = NULL;
	}

	service_enabled = false;

	return NULL;
}

static void mapLongPressed(myplace_app_data *ad)
{
	maps_coordinates_h coordinates = NULL;
	maps_view_object_h marker = NULL;
	int ret = 0;
	int request_id = 0;

	/* Find the address under the user long-press */
	ret = maps_service_reverse_geocode(ad->maps_service, ad->mapview_place->latitude, ad->mapview_place->longitude, NULL, addressOnMapsService_cb, ad, &request_id);
	if (ret != MAPS_ERROR_NONE) {
		LS_LOGE("maps_service_reverse_geocode fail, error = %d", ret);
		return;
	}

	/* Put a marker on the position, specified by the user */
	ret = maps_view_remove_all_objects(ad->maps_view);
	if (ret != MAPS_ERROR_NONE) {
		LS_LOGE("maps_view_remove_all_objects fail, error = %d", ret);
		return;
	}
	maps_coordinates_create(ad->mapview_place->latitude, ad->mapview_place->longitude, &coordinates);
	ret = maps_view_object_create_marker(coordinates, IMG_DIR"/marker_icon.png", MAPS_VIEW_MARKER_PIN, &marker);
	if (ret != MAPS_ERROR_NONE) {
		LS_LOGE("map_object_create_marker fail, error = %d", ret);
		return;
	}
	maps_coordinates_destroy(coordinates);
	ret = maps_view_add_object(ad->maps_view, marker);
	if (ret != MAPS_ERROR_NONE) {
		LS_LOGE("maps_view_add_object fail, error = %d", ret);
		return;
	}

	/* Stop the Location Manager */
	if (manager != NULL) {
		location_manager_stop(manager);
		location_manager_unset_position_updated_cb(manager);
		location_manager_unset_service_state_changed_cb(manager);
		location_manager_destroy(manager);
		manager = NULL;
		thread = NULL;
	}
}

static void start_get_position(void *data, Evas_Object *obj, void *event_info)
{
	myplace_app_data *ad = (myplace_app_data *)data;

	service_enabled = false;

	if (manager == NULL)
		location_manager_create(LOCATIONS_METHOD_HYBRID, &manager);
	location_manager_set_position_updated_cb(manager, position_updated, 1, ad);
	location_manager_set_service_state_changed_cb(manager, __state_changed_cb, NULL);
	location_manager_start(manager);

	if (thread == NULL)
		thread = g_thread_new(NULL, wait_for_service, ad);
}

static void maps_view_event_cb(maps_view_event_type_e type, maps_view_event_data_h event_data, void *user_data)
{
	myplace_app_data *ad = (myplace_app_data *)user_data;
	maps_view_gesture_e gesture_type = MAPS_VIEW_GESTURE_NONE;
	int ret = MAPS_ERROR_NONE;

	LS_LOGE("maps_view_event_cb enter");

	maps_view_event_data_get_gesture_type(event_data, &gesture_type);

	switch (type) {
	case MAPS_VIEW_EVENT_GESTURE:
		LS_LOGE("MAPS_VIEW_EVENT_GESTURE");
		switch (gesture_type) {
		case MAPS_VIEW_GESTURE_LONG_PRESS: {
			LS_LOGE("MAPS_VIEW_GESTURE_LONG_PRESS");
			maps_coordinates_h event_coord = NULL;
			int x = 0, y = 0;
			double latitude = 0.0, longitude = 0.0;
			ret = maps_view_event_data_get_position(event_data, &x, &y);
			if (ret != MAPS_ERROR_NONE) {
				LS_LOGE("maps_view_event_data_get_position is fail[%d]", ret);
				break;
			}
			ret = maps_view_screen_to_geolocation(ad->maps_view, x, y, &event_coord);
			if (ret != MAPS_ERROR_NONE) {
				LS_LOGE("maps_view_screen_to_geolocation is fail[%d]", ret);
				break;
			}
			ret = maps_coordinates_get_latitude_longitude(event_coord, &latitude, &longitude);
			if (ret != MAPS_ERROR_NONE) {
				LS_LOGE("maps_coordinates_get_latitude_longitude is fail[%d]", ret);
				break;
			}

			/* Prepare application data for long-press event processing */
			ad->mapview_place->latitude = latitude;
			ad->mapview_place->longitude = longitude;

			/* Process the long-press event */
			mapLongPressed(ad);

			/* Relaase event coordinates */
			maps_coordinates_destroy(event_coord);
			}
			break;
		default:
			break;
		}
		break;
	default:
		LS_LOGE("default type");
		break;
	}
	maps_view_event_data_destroy(event_data);
}

static bool searchOnMapsService_cb(maps_error_e result, int request_id, int index, int total, maps_coordinates_h coordinates, void *user_data)
{
	myplace_app_data *ad = (myplace_app_data *)user_data;
	maps_view_object_h marker = NULL;
	int ret = 0;

	LS_LOGE("searchOnMapsService_cb enter");

	if (result != MAPS_ERROR_NONE)
		return false;

	ret = maps_view_remove_all_objects(ad->maps_view);
	if (ret != MAPS_ERROR_NONE) {
		LS_LOGE("maps_view_remove_all_objects fail, error = %d", ret);
		return false;
	}
	ret = maps_view_set_zoom_level(ad->maps_view, 12);
	if (ret != MAPS_ERROR_NONE) {
		LS_LOGE("maps_view_set_zoom_level fail, error = %d", ret);
		return false;
	}
	ret = maps_view_set_center(ad->maps_view, coordinates);
	if (ret != MAPS_ERROR_NONE) {
		LS_LOGE("maps_view_set_center fail, error = %d", ret);
		return false;
	}
	ret = maps_view_object_create_marker(coordinates, IMG_DIR"/marker_icon.png", MAPS_VIEW_MARKER_PIN, &marker);
	if (ret != MAPS_ERROR_NONE) {
		LS_LOGE("map_object_create_marker fail, error = %d", ret);
		return false;
	}
	ret = maps_view_add_object(ad->maps_view, marker);
	if (ret != MAPS_ERROR_NONE) {
		LS_LOGE("maps_view_add_object fail, error = %d", ret);
		return false;
	}

	ad->coordinate = coordinates;

	elm_entry_input_panel_hide(ad->map_entry);

	/* Stop the Location Manager */
	if (manager != NULL) {
		location_manager_stop(manager);
		location_manager_unset_position_updated_cb(manager);
		location_manager_unset_service_state_changed_cb(manager);
		location_manager_destroy(manager);
		manager = NULL;
		thread = NULL;
	}

	return true;
}

static void searchPressedOnMaps(void *data, Evas_Object *obj, void *event_info)
{
	myplace_app_data *ad = (myplace_app_data *)data;
	maps_preference_h preference = NULL;
	int ret = 0;
	int request_id = 0;

	if (ad == NULL)
		return;
	if (elm_entry_entry_get(ad->map_entry) == NULL)
		toast_popup(ad, P_("IDS_LBS_NPBODY_NO_RESULTS_FOUND"));
	else {
		ret = maps_preference_create(&preference);
		if (ret != MAPS_ERROR_NONE) {
			LS_LOGE("maps_preference_create fail, error = %d", ret);
			return;
		}
		ret = maps_service_geocode(ad->maps_service, elm_entry_entry_get(obj), preference, searchOnMapsService_cb, ad, &request_id);
		if (ret != MAPS_ERROR_NONE) {
			LS_LOGE("maps_service_geocode fail, error = %d", ret);
			return;
		}
		ret = maps_preference_destroy(preference);
		if (ret != MAPS_ERROR_NONE) {
			LS_LOGE("maps_preference_destroy fail, error = %d", ret);
			return;
		}
	}
}

static void mapview_cancel_cb(void *data, Evas_Object * obj, void *event_info)
{
	myplace_app_data *ad = (myplace_app_data *)data;
	int ret = 0;

	if (ad->mapview_place != NULL) {
		if (ad->mapview_place->address != NULL)
			free(ad->mapview_place->address);
		free(ad->mapview_place);
		ad->mapview_place = NULL;
	}

	if (ad->maps_view != NULL) {

		/* Unset all callbacks */
		ret = maps_view_unset_event_cb(ad->maps_view, MAPS_VIEW_EVENT_GESTURE);
		if (ret != MAPS_ERROR_NONE)
			LS_LOGE("maps_view_unset_event_cb fail, error = %d", ret);
		ret = maps_view_unset_event_cb(ad->maps_view, MAPS_VIEW_EVENT_OBJECT);
		if (ret != MAPS_ERROR_NONE)
			LS_LOGE("maps_view_unset_event_cb fail, error = %d", ret);

		/* Remove all visual objects */
		ret = maps_view_remove_all_objects(ad->maps_view);

		/* Destroy the Map View */
		ret = maps_view_destroy(ad->maps_view);
		ad->maps_view = NULL;
	}
	if (ad->maps_service != NULL) {
		ret = maps_service_destroy(ad->maps_service);
		ad->maps_service = NULL;
	}
	if (manager != NULL) {
		location_manager_stop(manager);
		location_manager_unset_position_updated_cb(manager);
		location_manager_unset_service_state_changed_cb(manager);
		location_manager_destroy(manager);
		manager = NULL;
		thread = NULL;
	}

	eext_object_event_callback_del(ad->nf, EEXT_CALLBACK_BACK, mapview_cancel_cb);
	elm_naviframe_item_pop(ad->nf);
}

static void mapview_done_cb(void *data, Evas_Object * obj, void *event_info)
{
	myplace_app_data *ad = (myplace_app_data *)data;
	double lat = 0.0, lon = 0.0;
	char *address = NULL;

	elm_naviframe_item_pop(ad->nf);

	lat = ad->mapview_place->latitude;
	lon = ad->mapview_place->longitude;
	address = ad->mapview_place->address;

	LS_LOGE("lat=%lf lon=%lf address=%s", lat, lon, address);

	setPosition(MYPLACE_METHOD_MAP, lat, lon, address, ad);

	if (ad->mapview_place != NULL) {
		if (ad->mapview_place->address != NULL)
			free(ad->mapview_place->address);
		free(ad->mapview_place);
		ad->mapview_place = NULL;
	}

	if (ad->maps_view != NULL) {
		maps_view_remove_all_objects(ad->maps_view);
		maps_view_destroy(ad->maps_view);
		ad->maps_view = NULL;
	}
	if (ad->maps_service != NULL) {
		maps_service_destroy(ad->maps_service);
		ad->maps_service = NULL;
	}

	if (manager != NULL) {
		location_manager_stop(manager);
		location_manager_unset_position_updated_cb(manager);
		location_manager_unset_service_state_changed_cb(manager);
		location_manager_destroy(manager);
		manager = NULL;
		thread = NULL;
	}

	eext_object_event_callback_del(ad->nf, EEXT_CALLBACK_BACK, mapview_cancel_cb);
	elm_genlist_item_update(ad->gi_map_method);
}

static void gpsbutton_press_cb(void *data, Evas_Object *obj, void *event_info)
{
	Evas_Object *edje = (Evas_Object *)elm_layout_edje_get((Evas_Object *)data);
	edje_object_signal_emit(edje, "press", "button");
}

static void gpsbutton_unpress_cb(void *data, Evas_Object *obj, void *event_info)
{
	Evas_Object *edje = (Evas_Object *)elm_layout_edje_get((Evas_Object *)data);
	edje_object_signal_emit(edje, "normal", "button");
}

/*
static void gpsbutton_dim_cb(void *data, Evas_Object *obj, void *event_info)
{
	Evas_Object *edje = (Evas_Object *)elm_layout_edje_get((Evas_Object *)data);
	edje_object_signal_emit(edje, "dim", "button");
}
*/

static void searchentry_focused_cb(void *data, Evas_Object *obj, void *event_info)
{
	Evas_Object *editfield = (Evas_Object *)data;
	elm_object_signal_emit(editfield, "elm,state,focused", "");

	if (!elm_entry_is_empty(obj))
		elm_object_signal_emit(editfield, "elm,action,show,button", "");
}

static void searchentry_unfocused_cb(void *data, Evas_Object *obj, void *event_info)
{
	Evas_Object *editfield = (Evas_Object *)data;
	elm_object_signal_emit(editfield, "elm,state,unfocused", "");
	elm_object_signal_emit(editfield, "elm,action,hide,button", "");
}

static void searchentry_changed_cb(void *data, Evas_Object *obj, void *event_info)
{
	Evas_Object *editfield = (Evas_Object *)data;
	myplace_app_data *ad = evas_object_data_get(obj, "app_data");

	if (!elm_entry_is_empty(obj) && elm_object_focus_get(obj))
		elm_object_signal_emit(editfield, "elm,action,show,button", "");
	else
		elm_object_signal_emit(editfield, "elm,action,hide,button", "");

	if (elm_entry_is_empty(obj))
		elm_object_disabled_set(ad->map_done_btn, EINA_TRUE);
}

static void searchentry_clear_button_clicked_cb(void *data, Evas_Object *obj, void *event_info)
{
	myplace_app_data *ad = (myplace_app_data *)data;

	ad->mapview_place->latitude = -1;
	ad->mapview_place->longitude = -1;

	if (ad->mapview_place->address != NULL) {
		free(ad->mapview_place->address);
		ad->mapview_place->address = NULL;
	}
	elm_entry_entry_set(ad->map_entry, "");
	elm_object_disabled_set(ad->map_done_btn, EINA_TRUE);
}

Evas_Object *create_gpsbutton_layout(void *data, Evas_Object *obj)
{
	myplace_app_data *ad = (myplace_app_data *)data;

	Evas_Object *gps_button = elm_button_add(obj);
	elm_object_style_set(gps_button, "circle");
	evas_object_repeat_events_set(gps_button, EINA_FALSE);
	evas_object_propagate_events_set(gps_button, EINA_FALSE);
	evas_object_size_hint_weight_set(gps_button, 0.0, 0.0);
	evas_object_size_hint_align_set(gps_button, 0.5, 0.5);
	evas_object_show(gps_button);

	Evas_Object *layout = elm_layout_add(gps_button);
	elm_layout_file_set(layout, EDJ_DIR"/map_view.edj", "map_object");
	elm_object_content_set(gps_button, layout);

	evas_object_smart_callback_add(gps_button, "pressed", gpsbutton_press_cb, layout);
	evas_object_smart_callback_add(gps_button, "unpressed", gpsbutton_unpress_cb, layout);
	evas_object_smart_callback_add(gps_button, "clicked", start_get_position, ad);

	return gps_button;
}

static Evas_Image *create_map_layout(Evas_Object *parent, myplace_app_data *ad)
{
	Evas_Image *panel = evas_object_image_filled_add(evas_object_evas_get(parent));
	maps_coordinates_h maps_coord = NULL;
	maps_view_object_h marker = NULL;
	int ret = 0;

	ret = maps_service_create("HERE", &(ad->maps_service));
	if (ret != MAPS_ERROR_NONE) {
		LS_LOGE("maps_service_create fail, error = %d", ret);
		return NULL;
	}
	ret = maps_service_set_provider_key(ad->maps_service, "pE-W9LeqN7zB9RtnwgBN/tZuCgj-LtWQ4RWN56XrVpA");
	if (ret != MAPS_ERROR_NONE) {
		LS_LOGE("maps_service_set_provider_key fail, error = %d", ret);
		return NULL;
	}
	ret = maps_view_create(ad->maps_service, panel, &(ad->maps_view));
	if (ret != MAPS_ERROR_NONE) {
		LS_LOGE("maps_view_create fail, error = %d", ret);
		return NULL;
	}

	if (ad->mapview_place->address == NULL) {
		ret = maps_coordinates_create(0, 0, &maps_coord);
		if (ret != MAPS_ERROR_NONE) {
			LS_LOGE("maps_coordinates_create fail, error = %d", ret);
			return NULL;
		}
		maps_view_set_zoom_level(ad->maps_view, 2);
	} else {
		LS_LOGE("create_map_layout: lat=%lf lon=%lf address=%s", ad->mapview_place->latitude, ad->mapview_place->longitude, ad->mapview_place->address);

		ret = maps_coordinates_create(ad->mapview_place->latitude, ad->mapview_place->longitude, &maps_coord);
		if (ret != MAPS_ERROR_NONE) {
			LS_LOGE("maps_coordinates_create fail, error = %d", ret);
			return NULL;
		}
		maps_view_set_zoom_level(ad->maps_view, 12);

		if (ad->mapview_place->address != NULL)
			elm_entry_entry_set(ad->map_entry, ad->mapview_place->address);

		ret = maps_view_object_create_marker(maps_coord, IMG_DIR"/marker_icon.png", MAPS_VIEW_MARKER_PIN, &marker);
		if (ret != MAPS_ERROR_NONE) {
			LS_LOGE("map_object_create_marker fail, error = %d", ret);
			return NULL;
		}
		ret = maps_view_add_object(ad->maps_view, marker);
		if (ret != MAPS_ERROR_NONE) {
			LS_LOGE("maps_view_add_object fail, error = %d", ret);
			return NULL;
		}
	}
	ret = maps_view_set_center(ad->maps_view, maps_coord);
	if (ret != MAPS_ERROR_NONE) {
		LS_LOGE("maps_view_set_center fail, error = %d", ret);
		return NULL;
	}

	ret = maps_view_set_event_cb(ad->maps_view, MAPS_VIEW_EVENT_GESTURE, maps_view_event_cb, ad);
	if (ret != MAPS_ERROR_NONE) {
		LS_LOGE("maps_view_set_event_cb fail, error = %d", ret);
		return NULL;
	}
	ret = maps_view_set_event_cb(ad->maps_view, MAPS_VIEW_EVENT_OBJECT, maps_view_event_cb, ad);
	if (ret != MAPS_ERROR_NONE) {
		LS_LOGE("maps_view_set_event_cb fail, error = %d", ret);
		return NULL;
	}

	if (maps_coord != NULL)
		maps_coordinates_destroy(maps_coord);

	return panel;
}

static Evas_Object *create_searchfield_layout(myplace_app_data *ad, Evas_Object *parent)
{
	Evas_Object *layout = elm_layout_add(parent);
	elm_layout_theme_set(layout, "layout", "editfield", "singleline");
	evas_object_size_hint_align_set(layout, EVAS_HINT_FILL, 0.0);
	evas_object_size_hint_weight_set(layout, 0.9, 0.0);

	ad->map_entry = elm_entry_add(layout);
	elm_entry_single_line_set(ad->map_entry, EINA_TRUE);
	elm_entry_scrollable_set(ad->map_entry, EINA_TRUE);
	elm_entry_entry_set(ad->map_entry, "");
	evas_object_data_set(ad->map_entry, "app_data", ad);
	evas_object_smart_callback_add(ad->map_entry, "focused", searchentry_focused_cb, layout);
	evas_object_smart_callback_add(ad->map_entry, "unfocused", searchentry_unfocused_cb, layout);
	evas_object_smart_callback_add(ad->map_entry, "changed", searchentry_changed_cb, layout);
	evas_object_smart_callback_add(ad->map_entry, "preedit,changed", searchentry_changed_cb, layout);
	evas_object_smart_callback_add(ad->map_entry, "activated", searchPressedOnMaps, ad);
	elm_object_part_content_set(layout, "elm.swallow.content", ad->map_entry);
	elm_entry_input_panel_return_key_type_set(ad->map_entry, ELM_INPUT_PANEL_RETURN_KEY_TYPE_SEARCH);

	Evas_Object *del_button = elm_button_add(layout);
	elm_object_style_set(del_button, "editfield_clear");
	evas_object_smart_callback_add(del_button, "clicked", searchentry_clear_button_clicked_cb, ad);
	elm_object_part_content_set(layout, "elm.swallow.button", del_button);

	return layout;
}

static Evas_Object *create_map_view(myplace_app_data *ad)
{
	Evas_Object *main_scroller, *main_box, *searchfield, *layout, *sub_box, *button_layout;

	main_scroller = elm_scroller_add(ad->nf);
	elm_scroller_bounce_set(main_scroller, EINA_FALSE, EINA_TRUE);
	evas_object_size_hint_weight_set(main_scroller, EVAS_HINT_EXPAND, EVAS_HINT_EXPAND);
	evas_object_size_hint_align_set(main_scroller, EVAS_HINT_FILL, EVAS_HINT_FILL);
	evas_object_show(main_scroller);

	Evas_Object *bg_map_view = elm_bg_add(main_scroller);
	evas_object_size_hint_weight_set(bg_map_view, EVAS_HINT_EXPAND, EVAS_HINT_EXPAND);
	evas_object_size_hint_align_set(bg_map_view, EVAS_HINT_FILL, EVAS_HINT_FILL);
	elm_bg_color_set(bg_map_view, 255, 255, 255);
	evas_object_show(bg_map_view);

	main_box = elm_box_add(main_scroller);
	evas_object_size_hint_align_set(main_box, EVAS_HINT_FILL, EVAS_HINT_FILL);
	evas_object_size_hint_weight_set(main_box, EVAS_HINT_EXPAND, EVAS_HINT_EXPAND);
	evas_object_show(main_box);

	sub_box = elm_box_add(main_box);
	elm_box_horizontal_set(sub_box, EINA_TRUE);
	evas_object_size_hint_align_set(sub_box, EVAS_HINT_FILL, 0.0);
	evas_object_size_hint_weight_set(sub_box, EVAS_HINT_EXPAND, 0.0);
	evas_object_show(main_box);

	searchfield = create_searchfield_layout(ad, sub_box);
	elm_box_pack_end(sub_box, searchfield);
	evas_object_show(searchfield);

	button_layout = create_gpsbutton_layout(ad, sub_box);
	elm_box_pack_end(sub_box, button_layout);
	evas_object_show(button_layout);

	elm_box_pack_end(main_box, sub_box);
	evas_object_show(sub_box);

	layout = create_map_layout(main_box, ad);
	evas_object_size_hint_weight_set(layout, EVAS_HINT_EXPAND, EVAS_HINT_EXPAND);
	evas_object_size_hint_align_set(layout, EVAS_HINT_FILL, EVAS_HINT_FILL);
	elm_box_pack_end(main_box, layout);
	evas_object_show(layout);

	elm_object_part_content_set(bg_map_view, "overlay", main_box);
	elm_object_content_set(main_scroller, bg_map_view);

	return main_scroller;
}

void mapview(myplace_app_data *ad, myplace_data *place_nd)
{
	LS_FUNC_ENTER

	Evas_Object *nf = ad->nf;
	Elm_Object_Item *nf_it;

	ad->mapview_place = place_nd;

	if (manager != NULL) {
		location_manager_stop(manager);
		location_manager_unset_position_updated_cb(manager);
		location_manager_unset_service_state_changed_cb(manager);
		location_manager_destroy(manager);
		manager = NULL;
		thread = NULL;
	}

	Evas_Object *layout = create_map_view(ad);

	nf_it = elm_naviframe_item_push(nf, P_("IDS_LBS_BODY_MAP"), NULL, NULL, layout, NULL);
	eext_object_event_callback_add(ad->nf, EEXT_CALLBACK_BACK, mapview_cancel_cb, ad);

	/* title cancel button */
	ad->map_cancel_btn = elm_button_add(nf);
	elm_object_style_set(ad->map_cancel_btn, "naviframe/title_left");
	elm_object_part_text_set(ad->map_cancel_btn, "default", P_("IDS_COM_SK_CANCEL"));
	evas_object_smart_callback_add(ad->map_cancel_btn, "clicked", mapview_cancel_cb, ad);
	elm_object_item_part_content_set(nf_it, "title_left_btn", ad->map_cancel_btn);

	/* title done button */
	ad->map_done_btn = elm_button_add(nf);
	elm_object_style_set(ad->map_done_btn, "naviframe/title_right");
	elm_object_part_text_set(ad->map_done_btn, "default", P_("IDS_COM_BODY_DONE"));
	evas_object_smart_callback_add(ad->map_done_btn, "clicked", mapview_done_cb, ad);
	elm_object_item_part_content_set(nf_it, "title_right_btn", ad->map_done_btn);

	if (ad->mapview_place->address == NULL)
		elm_object_disabled_set(ad->map_done_btn, EINA_TRUE);
	else
		elm_object_disabled_set(ad->map_done_btn, EINA_FALSE);
}
