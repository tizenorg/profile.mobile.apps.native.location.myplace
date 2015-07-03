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

static void address_cb(void *data, Evas_Object *map, Elm_Map_Name *name)
{
	myplace_app_data *ad = (myplace_app_data *)data;
	const char *result_address = NULL;

	if (ad->mapview_place->address != NULL) {
		free(ad->mapview_place->address);
		ad->mapview_place->address = NULL;
	}

	result_address = elm_map_name_address_get(name);

	if (result_address == NULL) {
		char address[50] = {};
		snprintf(address, sizeof(address), "%lf, %lf", ad->mapview_place->latitude, ad->mapview_place->longitude);
		ad->mapview_place->address = strdup(address);
	} else
		ad->mapview_place->address = strdup(result_address);

	elm_entry_entry_set(ad->map_entry, ad->mapview_place->address);
	elm_object_disabled_set(ad->map_done_btn, EINA_FALSE);
}

static void showMarker(myplace_app_data *ad, double lon, double lat)
{
	Eina_List *ovl_list = elm_map_overlays_get(ad->map);
	Elm_Map_Overlay *ovl = NULL;

	if (ovl_list != NULL) {
		ovl = (Elm_Map_Overlay *)ovl_list->data;
		elm_map_overlay_del(ovl);
	}

	ovl = elm_map_overlay_add(ad->map, lon, lat);
	elm_map_overlay_region_set(ovl, lon, lat);
}

static void position_cb(void *data, Evas_Object *map, Elm_Map_Name *name)
{
	myplace_app_data *ad = (myplace_app_data *)data;

	elm_map_name_region_get(name, &(ad->mapview_place->longitude), &(ad->mapview_place->latitude));

	LS_LOGE("position_cb: lon=%lf, lat=%lf", ad->mapview_place->longitude, ad->mapview_place->latitude);

	if ((ad->mapview_place->longitude != 0) && (ad->mapview_place->latitude != 0)) {
		elm_map_zoom_set(ad->map, 12);
		elm_map_region_bring_in(ad->map, ad->mapview_place->longitude, ad->mapview_place->latitude);
		showMarker(ad, ad->mapview_place->longitude, ad->mapview_place->latitude);

		if (ad->mapview_place->address != NULL) {
			free(ad->mapview_place->address);
			ad->mapview_place->address = NULL;
		}
		if (elm_entry_entry_get(ad->map_entry) != NULL)
			ad->mapview_place->address = strdup(elm_entry_entry_get(ad->map_entry));
		elm_object_disabled_set(ad->map_done_btn, EINA_FALSE);
	} else {
		elm_object_disabled_set(ad->map_done_btn, EINA_TRUE);
		toast_popup(ad, P_("IDS_LBS_NPBODY_NO_RESULTS_FOUND"));
	}
}

static void showPosition(myplace_app_data *ad)
{
	elm_map_zoom_set(ad->map, 12);
	elm_map_region_bring_in(ad->map, ad->mapview_place->longitude, ad->mapview_place->latitude);

	showMarker(ad, ad->mapview_place->longitude, ad->mapview_place->latitude);
}

static void position_updated(double latitude, double longitude, double altitude, time_t timestamp, void *user_data)
{
	myplace_app_data *ad = (myplace_app_data *)user_data;

	char *address = NULL;

	if (service_enabled) {
		ad->mapview_place->latitude = latitude;
		ad->mapview_place->longitude = longitude;

		elm_map_name_add(ad->map, address, ad->mapview_place->longitude, ad->mapview_place->latitude, address_cb, ad);
		showPosition(ad);
	}
}

static gpointer wait_for_service(void *data)
{
	myplace_app_data *ad = (myplace_app_data *)data;
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

	if (timeout == 30)
		toast_popup(ad, "No results found.");

	return NULL;
}

static void showOriginPosition(void *data)
{
	myplace_app_data *ad = (myplace_app_data *)data;

	elm_map_zoom_set(ad->map, 12);
	elm_map_region_bring_in(ad->map, ad->mapview_place->longitude, ad->mapview_place->latitude);

	showMarker(ad, ad->mapview_place->longitude, ad->mapview_place->latitude);
	if (ad->mapview_place->address != NULL)
		elm_entry_entry_set(ad->map_entry, ad->mapview_place->address);
}

static void mapLongPressed(void *data, Evas_Object *obj, void *event_info)
{
	myplace_app_data *ad = (myplace_app_data *)data;
	Evas_Event_Mouse_Down *down = (Evas_Event_Mouse_Down *)event_info;

	elm_map_canvas_to_region_convert(ad->map, down->canvas.x, down->canvas.y, &(ad->mapview_place->longitude), &(ad->mapview_place->latitude));

	elm_map_zoom_set(ad->map, 12);
	elm_map_region_bring_in(ad->map, ad->mapview_place->longitude, ad->mapview_place->latitude);

	showMarker(ad, ad->mapview_place->longitude, ad->mapview_place->latitude);

	elm_map_name_add(ad->map, NULL, ad->mapview_place->longitude, ad->mapview_place->latitude, address_cb, ad);

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

static void searchPressed(void *data, Evas_Object *obj, void *event_info)
{
	myplace_app_data *ad = (myplace_app_data *)data;

	if (ad == NULL)
		return;
	if (elm_entry_entry_get(ad->map_entry) == NULL)
		toast_popup(ad, P_("IDS_LBS_NPBODY_NO_RESULTS_FOUND"));
	else
		elm_map_name_add(ad->map, elm_entry_entry_get(obj), 0, 0, position_cb, ad);

	if (manager != NULL) {
		location_manager_stop(manager);
		location_manager_unset_position_updated_cb(manager);
		location_manager_unset_service_state_changed_cb(manager);
		location_manager_destroy(manager);
		manager = NULL;
		thread = NULL;
	}
}

static void mapview_cancel_cb(void *data, Evas_Object * obj, void *event_info)
{
	myplace_app_data *ad = (myplace_app_data *)data;
	elm_naviframe_item_pop(ad->nf);

	free(ad->mapview_place);
	ad->mapview_place = NULL;

	if (manager != NULL) {
		location_manager_stop(manager);
		location_manager_unset_position_updated_cb(manager);
		location_manager_unset_service_state_changed_cb(manager);
		location_manager_destroy(manager);
		manager = NULL;
		thread = NULL;
	}
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

	if (ad->mapview_place->address != NULL)
		free(ad->mapview_place->address);
	free(ad->mapview_place);
	ad->mapview_place = NULL;

	if (manager != NULL) {
		location_manager_stop(manager);
		location_manager_unset_position_updated_cb(manager);
		location_manager_unset_service_state_changed_cb(manager);
		location_manager_destroy(manager);
		manager = NULL;
		thread = NULL;
	}

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
	elm_object_style_set(gps_button, "transparent");
	evas_object_repeat_events_set(gps_button, EINA_FALSE);
	evas_object_propagate_events_set(gps_button, EINA_FALSE);
	evas_object_size_hint_weight_set(gps_button, 0.1, 0.0);
	evas_object_size_hint_align_set(gps_button, 0.0, 0.3);
	evas_object_show(gps_button);

	Evas_Object *layout = elm_layout_add(gps_button);
	elm_layout_file_set(layout, EDJ_DIR"/map_view.edj", "map_object");
	evas_object_size_hint_align_set(layout, EVAS_HINT_FILL, 0.0);
	evas_object_size_hint_weight_set(layout, EVAS_HINT_EXPAND, 0.0);
	elm_object_content_set(gps_button, layout);

	evas_object_smart_callback_add(gps_button, "pressed", gpsbutton_press_cb, layout);
	evas_object_smart_callback_add(gps_button, "unpressed", gpsbutton_unpress_cb, layout);
	evas_object_smart_callback_add(gps_button, "clicked", start_get_position, ad);

	return gps_button;
}

static Evas_Object *create_map_layout(Evas_Object *parent, myplace_app_data *ad)
{
	Evas_Object *scroller;

	scroller = elm_scroller_add(parent);
	elm_scroller_bounce_set(scroller, EINA_FALSE, EINA_TRUE);
	elm_scroller_policy_set(scroller, ELM_SCROLLER_POLICY_OFF, ELM_SCROLLER_POLICY_AUTO);

	ad->map = elm_map_add(scroller);

	elm_map_zoom_mode_set(ad->map, ELM_MAP_ZOOM_MODE_MANUAL);

	if (ad->mapview_place->address == NULL) {
		elm_map_zoom_set(ad->map, 2);
		elm_map_region_show(ad->map, 0.0, 0.0);
	} else {
		LS_LOGE("create_map_layout: lat=%lf lon=%lf address=%s", ad->mapview_place->latitude, ad->mapview_place->longitude, ad->mapview_place->address);
		showOriginPosition(ad);
	}

	evas_object_smart_callback_add(ad->map, "longpressed", mapLongPressed, ad);

	elm_object_content_set(scroller, ad->map);

	return scroller;
}

static Evas_Object *create_searchfield_layout(myplace_app_data *ad, Evas_Object *parent)
{
	Evas_Object *layout = elm_layout_add(parent);
	elm_layout_theme_set(layout, "layout", "searchfield", "singleline");
	evas_object_size_hint_align_set(layout, EVAS_HINT_FILL, 0.0);
	evas_object_size_hint_weight_set(layout, EVAS_HINT_EXPAND, 0.0);

	ad->map_entry = elm_entry_add(layout);
	elm_entry_single_line_set(ad->map_entry, EINA_TRUE);
	elm_entry_scrollable_set(ad->map_entry, EINA_TRUE);
	elm_entry_entry_set(ad->map_entry, "");
	evas_object_data_set(ad->map_entry, "app_data", ad);
	evas_object_smart_callback_add(ad->map_entry, "focused", searchentry_focused_cb, layout);
	evas_object_smart_callback_add(ad->map_entry, "unfocused", searchentry_unfocused_cb, layout);
	evas_object_smart_callback_add(ad->map_entry, "changed", searchentry_changed_cb, layout);
	evas_object_smart_callback_add(ad->map_entry, "preedit,changed", searchentry_changed_cb, layout);
	evas_object_smart_callback_add(ad->map_entry, "activated", searchPressed, ad);
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

	elm_object_content_set(main_scroller, main_box);

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

	/* title cancel button */
	ad->map_cancel_btn = elm_button_add(nf);
	/* elm_object_style_set(ad->map_cancel_btn, "naviframe/title_text_left"); */
	elm_object_style_set(ad->map_cancel_btn, "naviframe/title_text");
	elm_object_part_text_set(ad->map_cancel_btn, "default", P_("IDS_COM_SK_CANCEL"));
	evas_object_smart_callback_add(ad->map_cancel_btn, "clicked", mapview_cancel_cb, ad);
	/* elm_object_item_part_content_set(nf_it, "title_left_text_btn", ad->map_cancel_btn); */
	elm_object_item_part_content_set(nf_it, "title_left_btn", ad->map_cancel_btn);

	/* title done button */
	ad->map_done_btn = elm_button_add(nf);
	/* elm_object_style_set(ad->map_done_btn, "naviframe/title_text_right"); */
	elm_object_style_set(ad->map_done_btn, "naviframe/title_text");
	elm_object_part_text_set(ad->map_done_btn, "default", P_("IDS_COM_BODY_DONE"));
	evas_object_smart_callback_add(ad->map_done_btn, "clicked", mapview_done_cb, ad);
	/* elm_object_item_part_content_set(nf_it, "title_right_text_btn", ad->map_done_btn); */
	elm_object_item_part_content_set(nf_it, "title_right_btn", ad->map_done_btn);

	if (ad->mapview_place->address == NULL)
		elm_object_disabled_set(ad->map_done_btn, EINA_TRUE);
	else
		elm_object_disabled_set(ad->map_done_btn, EINA_FALSE);
}
