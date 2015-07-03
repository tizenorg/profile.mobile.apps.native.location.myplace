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

#include "myplace-common.h"
#include "myplace-placelist.h"
#include "myplace-elementary.h"
#include "myplace-detailinfo.h"
#include "myplace-mapview.h"

void clear_place_method_data(myplace_data *temp)
{
	temp->latitude = -1;
	temp->longitude = -1;

	if (temp->address) {
		free(temp->address);
		temp->address = NULL;
	}

	if (temp->wifi_bssid) {
		free(temp->wifi_bssid);
		temp->wifi_bssid = NULL;
	}

	if (temp->wifi_ssid) {
		free(temp->wifi_ssid);
		temp->wifi_ssid = NULL;
	}

	if (temp->bt_mac_address) {
		free(temp->bt_mac_address);
		temp->bt_mac_address = NULL;
	}

	if (temp->bt_ssid) {
		free(temp->bt_ssid);
		temp->bt_ssid = NULL;
	}
}

void setPosition(myplace_method_index_e method, double lat, double lon, const char *address, myplace_app_data *ad)
{
	ad->selected_place->method_map = true;
	ad->selected_place->latitude = lat;
	ad->selected_place->longitude = lon;
	if (ad->selected_place->address != NULL) {
		free(ad->selected_place->address);
		ad->selected_place->address = NULL;
	}
	ad->selected_place->address = strdup(address);
}

static void
detailinfo_cancel_cb(void *data, Evas_Object * obj, void *event_info)
{
	myplace_app_data *ad = (myplace_app_data *)data;

	if (ad->selected_place->name)
		free(ad->selected_place->name);

	clear_place_method_data(ad->selected_place);

	free(ad->selected_place);
	ad->selected_place = NULL;

	ad->current_index = 0;
	elm_naviframe_item_pop(ad->nf);
}

static void
detailinfo_done_cb(void *data, Evas_Object * obj, void *event_info)
{
	myplace_app_data *ad = (myplace_app_data *)data;

	if (ad->selected_place->name) {
		int i = 0;
		bool usedName = false;

		for (; i <= ad->last_index; i++) {
			if ((i != ad->current_index) && (!strcmp(ad->selected_place->name, ad->placelist[i]->name)))
				usedName = true;
		}

		if (usedName) {
			toast_popup(ad, P_("IDS_ST_TPOP_PLACE_NAME_ALREADY_IN_USE"));
			return;
		}
		elm_naviframe_item_pop(ad->nf);
	} else {
		toast_popup(ad, P_("IDS_COM_BODY_NO_NAME"));
		return;
	}

	if (ad->current_index <= ad->last_index) {
		if (strcmp(ad->selected_place->name, ad->placelist[ad->current_index]->name) != 0) {
			free(ad->placelist[ad->current_index]->name);
			ad->placelist[ad->current_index]->name = strdup(ad->selected_place->name);
			geofence_manager_update_place(ad->geo_manager, ad->placelist[ad->current_index]->place_id, ad->placelist[ad->current_index]->name);
		}

		ad->placelist[ad->current_index]->is_default = ad->selected_place->is_default;

		if (ad->placelist[ad->current_index]->method_map == true) {
			if (ad->selected_place->method_map == true) {
				if (ad->placelist[ad->current_index]->latitude != ad->selected_place->latitude || ad->placelist[ad->current_index]->longitude != ad->selected_place->longitude) {
					geofence_manager_remove_fence(ad->geo_manager, ad->placelist[ad->current_index]->map_fence_id);
					geofence_destroy(ad->placelist[ad->current_index]->map_geofence_params);

					ad->placelist[ad->current_index]->map_fence_id = -1;
					ad->placelist[ad->current_index]->map_geofence_params = NULL;

					ad->placelist[ad->current_index]->latitude = ad->selected_place->latitude;
					ad->placelist[ad->current_index]->longitude = ad->selected_place->longitude;

					free(ad->placelist[ad->current_index]->address);
					ad->placelist[ad->current_index]->address = strdup(ad->selected_place->address);

					geofence_create_geopoint(ad->placelist[ad->current_index]->place_id, ad->placelist[ad->current_index]->latitude, ad->placelist[ad->current_index]->longitude, PLACE_GEOPOINT_RADIUS, ad->placelist[ad->current_index]->address, &(ad->placelist[ad->current_index]->map_geofence_params));
					geofence_manager_add_fence(ad->geo_manager, ad->placelist[ad->current_index]->map_geofence_params, &(ad->placelist[ad->current_index]->map_fence_id));
				}
			} else {
				geofence_manager_remove_fence(ad->geo_manager, ad->placelist[ad->current_index]->map_fence_id);
				geofence_destroy(ad->placelist[ad->current_index]->map_geofence_params);

				ad->placelist[ad->current_index]->map_fence_id = -1;
				ad->placelist[ad->current_index]->map_geofence_params = NULL;

				ad->placelist[ad->current_index]->method_map = false;
				ad->placelist[ad->current_index]->latitude = -1;
				ad->placelist[ad->current_index]->longitude = -1;
				free(ad->placelist[ad->current_index]->address);
				ad->placelist[ad->current_index]->address = NULL;
			}
		} else {
			if (ad->selected_place->method_map == true) {
				ad->placelist[ad->current_index]->latitude = ad->selected_place->latitude;
				ad->placelist[ad->current_index]->longitude = ad->selected_place->longitude;
				ad->placelist[ad->current_index]->address = strdup(ad->selected_place->address);

				geofence_create_geopoint(ad->placelist[ad->current_index]->place_id, ad->placelist[ad->current_index]->latitude, ad->placelist[ad->current_index]->longitude, PLACE_GEOPOINT_RADIUS, ad->placelist[ad->current_index]->address, &(ad->placelist[ad->current_index]->map_geofence_params));
				geofence_manager_add_fence(ad->geo_manager, ad->placelist[ad->current_index]->map_geofence_params, &(ad->placelist[ad->current_index]->map_fence_id));

				ad->placelist[ad->current_index]->method_map = true;
			}
		}

		if (ad->placelist[ad->current_index]->method_wifi == true) {
			if (ad->selected_place->method_wifi == true) {
				if (strcmp(ad->placelist[ad->current_index]->wifi_bssid, ad->selected_place->wifi_bssid) != 0) {
					geofence_manager_remove_fence(ad->geo_manager, ad->placelist[ad->current_index]->wifi_fence_id);
					geofence_destroy(ad->placelist[ad->current_index]->wifi_geofence_params);

					ad->placelist[ad->current_index]->wifi_fence_id = -1;
					ad->placelist[ad->current_index]->wifi_geofence_params = NULL;

					free(ad->placelist[ad->current_index]->wifi_bssid);
					free(ad->placelist[ad->current_index]->wifi_ssid);
					ad->placelist[ad->current_index]->wifi_bssid = strdup(ad->selected_place->wifi_bssid);
					ad->placelist[ad->current_index]->wifi_ssid = strdup(ad->selected_place->wifi_ssid);

					geofence_create_wifi(ad->placelist[ad->current_index]->place_id, ad->placelist[ad->current_index]->wifi_bssid, ad->placelist[ad->current_index]->wifi_ssid, &(ad->placelist[ad->current_index]->wifi_geofence_params));
					geofence_manager_add_fence(ad->geo_manager, ad->placelist[ad->current_index]->wifi_geofence_params, &(ad->placelist[ad->current_index]->wifi_fence_id));
				}
			} else {
				geofence_manager_remove_fence(ad->geo_manager, ad->placelist[ad->current_index]->wifi_fence_id);
				geofence_destroy(ad->placelist[ad->current_index]->wifi_geofence_params);

				ad->placelist[ad->current_index]->wifi_fence_id = -1;
				ad->placelist[ad->current_index]->wifi_geofence_params = NULL;

				ad->placelist[ad->current_index]->method_wifi = false;
				free(ad->placelist[ad->current_index]->wifi_bssid);
				free(ad->placelist[ad->current_index]->wifi_ssid);
				ad->placelist[ad->current_index]->wifi_bssid = NULL;
				ad->placelist[ad->current_index]->wifi_ssid = NULL;
			}
		} else {
			if (ad->selected_place->method_wifi == true) {
				ad->placelist[ad->current_index]->wifi_bssid = strdup(ad->selected_place->wifi_bssid);
				ad->placelist[ad->current_index]->wifi_ssid = strdup(ad->selected_place->wifi_ssid);

				geofence_create_wifi(ad->placelist[ad->current_index]->place_id, ad->placelist[ad->current_index]->wifi_bssid, ad->placelist[ad->current_index]->wifi_ssid, &(ad->placelist[ad->current_index]->wifi_geofence_params));
				geofence_manager_add_fence(ad->geo_manager, ad->placelist[ad->current_index]->wifi_geofence_params, &(ad->placelist[ad->current_index]->wifi_fence_id));

				ad->placelist[ad->current_index]->method_wifi = true;
			}
		}

		if (ad->placelist[ad->current_index]->method_bt == true) {
			if (ad->selected_place->method_bt == true) {
				if (strcmp(ad->placelist[ad->current_index]->bt_mac_address, ad->selected_place->bt_mac_address) != 0) {
					geofence_manager_remove_fence(ad->geo_manager, ad->placelist[ad->current_index]->bt_fence_id);
					geofence_destroy(ad->placelist[ad->current_index]->bt_geofence_params);

					ad->placelist[ad->current_index]->bt_fence_id = -1;
					ad->placelist[ad->current_index]->bt_geofence_params = NULL;

					free(ad->placelist[ad->current_index]->bt_mac_address);
					free(ad->placelist[ad->current_index]->bt_ssid);
					ad->placelist[ad->current_index]->bt_mac_address = strdup(ad->selected_place->bt_mac_address);
					ad->placelist[ad->current_index]->bt_ssid = strdup(ad->selected_place->bt_ssid);

					geofence_create_bluetooth(ad->placelist[ad->current_index]->place_id, ad->placelist[ad->current_index]->bt_mac_address, ad->placelist[ad->current_index]->bt_ssid, &(ad->placelist[ad->current_index]->bt_geofence_params));
					geofence_manager_add_fence(ad->geo_manager, ad->placelist[ad->current_index]->bt_geofence_params, &(ad->placelist[ad->current_index]->bt_fence_id));
				}
			} else {
				geofence_manager_remove_fence(ad->geo_manager, ad->placelist[ad->current_index]->bt_fence_id);
				geofence_destroy(ad->placelist[ad->current_index]->bt_geofence_params);

				ad->placelist[ad->current_index]->bt_fence_id = -1;
				ad->placelist[ad->current_index]->bt_geofence_params = NULL;

				ad->placelist[ad->current_index]->method_bt = false;
				free(ad->placelist[ad->current_index]->bt_mac_address);
				free(ad->placelist[ad->current_index]->bt_ssid);
				ad->placelist[ad->current_index]->bt_mac_address = NULL;
				ad->placelist[ad->current_index]->bt_ssid = NULL;
			}
		} else {
			if (ad->selected_place->method_bt == true) {
				ad->placelist[ad->current_index]->bt_mac_address = strdup(ad->selected_place->bt_mac_address);
				ad->placelist[ad->current_index]->bt_ssid = strdup(ad->selected_place->bt_ssid);

				geofence_create_bluetooth(ad->placelist[ad->current_index]->place_id, ad->placelist[ad->current_index]->bt_mac_address, ad->placelist[ad->current_index]->bt_ssid, &(ad->placelist[ad->current_index]->bt_geofence_params));
				geofence_manager_add_fence(ad->geo_manager, ad->placelist[ad->current_index]->bt_geofence_params, &(ad->placelist[ad->current_index]->bt_fence_id));

				ad->placelist[ad->current_index]->method_bt = true;
			}
		}

		elm_genlist_item_update(ad->placelist[ad->current_index]->gi_myplace);

		clear_place_method_data(ad->selected_place);
	} else {
		ad->last_index = ad->current_index;

		ad->placelist[ad->last_index] = ad->selected_place;

		evas_object_data_set(ad->place_genlist, "app_data", ad);
		ad->placelist[ad->last_index]->itc_myplace = elm_genlist_item_class_new();
		ad->placelist[ad->last_index]->itc_myplace->item_style = "2line.top";
		ad->placelist[ad->last_index]->itc_myplace->func.text_get = myplace_place_text_get;
		ad->placelist[ad->last_index]->itc_myplace->func.content_get = NULL;
		ad->placelist[ad->last_index]->itc_myplace->func.state_get = NULL;
		ad->placelist[ad->last_index]->itc_myplace->func.del = NULL;

		if (ad->last_index <= 0)
			ad->placelist[ad->last_index]->gi_myplace = elm_genlist_item_append(ad->place_genlist, ad->placelist[ad->last_index]->itc_myplace, (void *)ad->last_index, NULL, ELM_GENLIST_ITEM_NONE, placeinfo_cb, (void *)ad->last_index);
		else
			ad->placelist[ad->last_index]->gi_myplace = elm_genlist_item_insert_after(ad->place_genlist, ad->placelist[ad->last_index]->itc_myplace, (void *)ad->last_index, NULL, ad->placelist[ad->last_index-1]->gi_myplace, ELM_GENLIST_ITEM_NONE, placeinfo_cb, (void *)ad->last_index);

		geofence_manager_add_place(ad->geo_manager, ad->placelist[ad->current_index]->name, &(ad->placelist[ad->current_index]->place_id));

		if (ad->placelist[ad->current_index]->method_map == true) {
            geofence_create_geopoint(ad->placelist[ad->current_index]->place_id, ad->placelist[ad->current_index]->latitude, ad->placelist[ad->current_index]->longitude, PLACE_GEOPOINT_RADIUS, ad->placelist[ad->current_index]->address, &(ad->placelist[ad->current_index]->map_geofence_params));
            geofence_manager_add_fence(ad->geo_manager, ad->placelist[ad->current_index]->map_geofence_params, &(ad->placelist[ad->current_index]->map_fence_id));
        }

        if (ad->placelist[ad->current_index]->method_wifi == true) {
			geofence_create_wifi(ad->placelist[ad->current_index]->place_id, ad->placelist[ad->current_index]->wifi_bssid, ad->placelist[ad->current_index]->wifi_ssid, &(ad->placelist[ad->current_index]->wifi_geofence_params));
			geofence_manager_add_fence(ad->geo_manager, ad->placelist[ad->current_index]->wifi_geofence_params, &(ad->placelist[ad->current_index]->wifi_fence_id));
		}

		if (ad->placelist[ad->current_index]->method_bt == true) {
			geofence_create_bluetooth(ad->placelist[ad->current_index]->place_id, ad->placelist[ad->current_index]->bt_mac_address, ad->placelist[ad->current_index]->bt_ssid, &(ad->placelist[ad->current_index]->bt_geofence_params));
			geofence_manager_add_fence(ad->geo_manager, ad->placelist[ad->current_index]->bt_geofence_params, &(ad->placelist[ad->current_index]->bt_fence_id));
		}
	}
	ad->selected_place = NULL;
}

static char *myplace_place_name_text_get(void *data, Evas_Object *obj, const char *part)
{
	if (!g_strcmp0(part, "elm.text.main"))
		return strdup(P_("IDS_ST_BODY_NAME"));

	return NULL;
}

static char *myplace_discription_text_get(void *data, Evas_Object *obj, const char *part)
{
	if (!g_strcmp0(part, "elm.text.multiline"))
		return strdup(P_("IDS_ST_BODY_YOUR_DEVICE_WILL_PROVIDE_RELEVANT_INFORMATION_AND_SERVICES_USING_MAPS_WI_FI_OR_BLUETOOTH_WHEN_IT_RECOGNISES_THAT_YOU_ARE_AT_SAVED_LOCATIONS"));

	return NULL;
}

static char *myplace_fence_group_text_get(void *data, Evas_Object *obj, const char *part)
{
	if (!g_strcmp0(part, "elm.text.main"))
		return strdup(P_("IDS_DVBH_HEADER_SELECT_METHOD"));

	return NULL;
}

static void entry_activated_cb(void *data, Evas_Object * obj, void *event_info)
{
	myplace_app_data *ad = (myplace_app_data *)data;

	if (ad == NULL)
		return;
	if (ad->selected_place == NULL)
		return;
	if (ad->selected_place->name != NULL) {
		free(ad->selected_place->name);
		ad->selected_place->name = NULL;
	}

	if (elm_entry_entry_get(obj) != NULL)
		ad->selected_place->name = strdup(elm_entry_entry_get(obj));
}

static void editfield_focused_cb(void *data, Evas_Object *obj, void *event_info)
{
	Evas_Object *editfield = (Evas_Object *)data;
	elm_object_signal_emit(editfield, "elm,state,focused", "");

	if (!elm_entry_is_empty(obj))
		elm_object_signal_emit(editfield, "elm,action,show,button", "");
}

static void editfield_unfocused_cb(void *data, Evas_Object *obj, void *event_info)
{
	Evas_Object *editfield = (Evas_Object *)data;
	elm_object_signal_emit(editfield, "elm,state,unfocused", "");
	elm_object_signal_emit(editfield, "elm,action,hide,button", "");
}

static void editfield_changed_cb(void *data, Evas_Object *obj, void *event_info)
{
	Evas_Object *editfield = (Evas_Object *)data;

	if (!elm_entry_is_empty(obj) && elm_object_focus_get(obj))
		elm_object_signal_emit(editfield, "elm,action,show,button", "");
	else
		elm_object_signal_emit(editfield, "elm,action,hide,button", "");
}

static void editfield_clear_button_clicked_cb(void *data, Evas_Object *obj, void *event_info)
{
	Evas_Object *entry = (Evas_Object *)data;
	elm_entry_entry_set(entry, "");
}


static Evas_Object *myplace_place_name_content_get(void *data, Evas_Object *obj, const char *part)
{
	myplace_app_data *ad = evas_object_data_get(obj, "app_data");

	if (ad == NULL)
		return NULL;

	if (!strcmp(part, "elm.icon.entry")) {
		Evas_Object *editfield, *entry, *button;

		editfield = elm_layout_add(obj);
		elm_layout_theme_set(editfield, "layout", "editfield", "singleline");
		evas_object_size_hint_align_set(editfield, EVAS_HINT_FILL, 0.0);
		evas_object_size_hint_weight_set(editfield, EVAS_HINT_EXPAND, 0.0);

		entry = elm_entry_add(editfield);
		elm_entry_single_line_set(entry, EINA_TRUE);
		elm_entry_scrollable_set(entry, EINA_TRUE);

		if (ad->selected_place->name)
			elm_entry_entry_set(entry, ad->selected_place->name);
		else {
			char guide_text[20] = {};
			snprintf(guide_text, sizeof(guide_text), "My place %d", ad->last_index + 2);
			elm_object_part_text_set(entry, "guide", strdup(guide_text));
		}

		if (ad->selected_place->is_default)
			elm_entry_editable_set(entry, EINA_FALSE);
		else {
			elm_entry_editable_set(entry, EINA_TRUE);
			evas_object_smart_callback_add(entry, "changed,user", entry_activated_cb, ad);
		}

		evas_object_smart_callback_add(entry, "focused", editfield_focused_cb, editfield);
		evas_object_smart_callback_add(entry, "unfocused", editfield_unfocused_cb, editfield);
		evas_object_smart_callback_add(entry, "changed", editfield_changed_cb, editfield);
		evas_object_smart_callback_add(entry, "preedit,changed", editfield_changed_cb, editfield);
		elm_object_part_content_set(editfield, "elm.swallow.content", entry);

		button = elm_button_add(editfield);	elm_object_style_set(button, "editfield_clear");
		evas_object_smart_callback_add(button, "clicked", editfield_clear_button_clicked_cb, entry);
		elm_object_part_content_set(editfield, "elm.swallow.button", button);

		return editfield;
	}
	return NULL;
}

static char *myplace_select_map_text_get(void *data, Evas_Object *obj, const char *part)
{
	myplace_app_data *ad = evas_object_data_get(obj, "app_data");

	if (ad == NULL)
		return NULL;

	if (!g_strcmp0(part, "elm.text.main.left.top"))
		return strdup(P_("IDS_LBS_BODY_MAP"));

	if (!g_strcmp0(part, "elm.text.sub.left.bottom")) {
		if (ad->selected_place->address == NULL)
			return strdup(P_("IDS_COM_BODY_NONE"));
		else
			return strdup(ad->selected_place->address);
	}
	return NULL;
}

static char *myplace_select_wifi_text_get(void *data, Evas_Object *obj, const char *part)
{
	myplace_app_data *ad = evas_object_data_get(obj, "app_data");

	if (ad == NULL)
		return NULL;

	if (!g_strcmp0(part, "elm.text.main.left.top"))
		return strdup(P_("IDS_COM_BODY_WI_FI"));

	if (!g_strcmp0(part, "elm.text.sub.left.bottom")) {
		if (ad->selected_place->wifi_bssid == NULL)
			return strdup(P_("IDS_COM_BODY_NONE"));
		else
			return strdup(ad->selected_place->wifi_ssid);
	}
	return NULL;
}

static char *myplace_select_bt_text_get(void *data, Evas_Object *obj, const char *part)
{
	myplace_app_data *ad = evas_object_data_get(obj, "app_data");

	if (ad == NULL)
		return NULL;

	if (!g_strcmp0(part, "elm.text.main.left.top"))
		return strdup(P_("IDS_COM_BODY_BLUETOOTH"));

	if (!g_strcmp0(part, "elm.text.sub.left.bottom")) {
		if (ad->selected_place->bt_mac_address == NULL)
			return strdup(P_("IDS_COM_BODY_NONE"));
		else
			return strdup(ad->selected_place->bt_ssid);
	}
	return NULL;
}

static void delbutton_press_cb(void *data, Evas_Object *obj, void *event_info)
{
	Evas_Object *edje = (Evas_Object *)elm_layout_edje_get((Evas_Object *)data);
	edje_object_signal_emit(edje, "press", "button");
}

static void delbutton_unpress_cb(void *data, Evas_Object *obj, void *event_info)
{
	Evas_Object *edje = (Evas_Object *)elm_layout_edje_get((Evas_Object *)data);
	edje_object_signal_emit(edje, "normal", "button");
}

/*
static void delbutton_dim_cb(void *data, Evas_Object *obj, void *event_info)
{
    Evas_Object *edje = (Evas_Object *)elm_layout_edje_get((Evas_Object *)data);
    edje_object_signal_emit(edje, "dim", "button");
}
*/

static void del_button_cb(void *data, Evas_Object *obj, void *event_info)
{
	myplace_app_data *ad = evas_object_data_get(obj, "app_data");
	int method = (int) data;

	if (ad == NULL)
		return;

	if (method == MYPLACE_METHOD_MAP) {
		ad->selected_place->method_map = false;
		ad->selected_place->latitude = -1;
		ad->selected_place->longitude = -1;
		free(ad->selected_place->address);
		ad->selected_place->address = NULL;
		elm_genlist_item_update(ad->gi_map_method);
	} else if (method == MYPLACE_METHOD_WIFI) {
		ad->selected_place->method_wifi = false;
		free(ad->selected_place->wifi_bssid);
		free(ad->selected_place->wifi_ssid);
		ad->selected_place->wifi_bssid = NULL;
		ad->selected_place->wifi_ssid = NULL;
		elm_genlist_item_update(ad->gi_wifi_method);
	} else if (method == MYPLACE_METHOD_BT) {
		ad->selected_place->method_bt = false;
		free(ad->selected_place->bt_mac_address);
		free(ad->selected_place->bt_ssid);
		ad->selected_place->bt_mac_address = NULL;
		ad->selected_place->bt_ssid = NULL;
		elm_genlist_item_update(ad->gi_bt_method);
	}
}

static Evas_Object *gl_button_content_get_cb(void *data, Evas_Object *obj, const char *part)
{
	myplace_app_data *ad = evas_object_data_get(obj, "app_data");
	int method = (int) data;

	if (!strcmp(part, "elm.icon.right")) {
		if ((method == MYPLACE_METHOD_MAP && ad->selected_place->method_map == true) ||
			(method == MYPLACE_METHOD_WIFI && ad->selected_place->method_wifi == true) ||
			(method == MYPLACE_METHOD_BT && ad->selected_place->method_bt == true)) {

			Evas_Object *del_button = elm_button_add(obj);
			evas_object_data_set(del_button, "app_data", ad);

			evas_object_repeat_events_set(del_button, EINA_FALSE);
			evas_object_propagate_events_set(del_button, EINA_FALSE);

			/* elm_object_style_set(del_button, "transparent"); */
			evas_object_show(del_button);

			Evas_Object *layout = elm_layout_add(del_button);
			elm_layout_file_set(layout, EDJ_DIR"/detail_view.edj", "delete_object");
			evas_object_repeat_events_set(layout, EINA_FALSE);
			evas_object_propagate_events_set(layout, EINA_FALSE);

			evas_object_size_hint_aspect_set(layout, EVAS_ASPECT_CONTROL_VERTICAL, 1, 1);
			evas_object_show(layout);
			elm_object_content_set(del_button, layout);

			evas_object_smart_callback_add(del_button, "pressed", delbutton_press_cb, layout);
			evas_object_smart_callback_add(del_button, "unpressed", delbutton_unpress_cb, layout);
			evas_object_smart_callback_add(del_button, "clicked", del_button_cb, (void *)method);

			return del_button;
			}
	}
	return NULL;
}

static void service_wifi_reply_cb(app_control_h request, app_control_h reply, app_control_result_e result, void *user_data)
{
	LS_FUNC_ENTER

	myplace_app_data *ad = (myplace_app_data *)user_data;

	int ret = -1;

	char *bssid = NULL;
	char *ssid = NULL;

	ret = app_control_get_extra_data(reply, "bssid", &bssid);

	if (ret != APP_CONTROL_ERROR_NONE) {
		LS_LOGD("ERROR BSSID %d", ret);
		return;
	}

	ret = app_control_get_extra_data(reply, "ssid", &ssid);

	if (ret != APP_CONTROL_ERROR_NONE) {
		LS_LOGD("ERROR SSID %d", ret);
		return;
	}

	if (ad->selected_place->wifi_bssid) {
		free(ad->selected_place->wifi_bssid);
		ad->selected_place->wifi_bssid = NULL;
	}
	ad->selected_place->wifi_bssid = strdup(bssid);

	if (ad->selected_place->wifi_ssid) {
		free(ad->selected_place->wifi_ssid);
		ad->selected_place->wifi_ssid = NULL;
	}
	ad->selected_place->wifi_ssid = strdup(ssid);

	ad->selected_place->method_wifi = true;

	elm_genlist_item_update(ad->gi_wifi_method);
}

static void service_bluetooth_reply_cb(app_control_h request, app_control_h reply, app_control_result_e result, void *user_data)
{
	LS_FUNC_ENTER

	myplace_app_data *ad = (myplace_app_data *)user_data;

	int ret = -1;
	char *name = NULL, *address = NULL;

	ret = app_control_get_extra_data(reply, "address", &address);

	if (ret != APP_CONTROL_ERROR_NONE) {
		LS_LOGD("ERROR ADDRESS %d", ret);
		return;
	}

	ret = app_control_get_extra_data(reply, "name", &name);

	if (ret != APP_CONTROL_ERROR_NONE) {
		LS_LOGD("ERROR NAME %d", ret);
		return;
	}

	if (ad->selected_place->bt_mac_address) {
		free(ad->selected_place->bt_mac_address);
		ad->selected_place->bt_mac_address = NULL;
	}
	ad->selected_place->bt_mac_address = strdup(address);

	if (ad->selected_place->bt_ssid) {
		free(ad->selected_place->bt_ssid);
		ad->selected_place->bt_ssid = NULL;
	}
	ad->selected_place->bt_ssid = strdup(name);

	ad->selected_place->method_bt = true;

	elm_genlist_item_update(ad->gi_bt_method);
}

static void select_map_method_cb(void *data, Evas_Object *obj, void *event_info)
{
	LS_FUNC_ENTER

	myplace_app_data *ad = (myplace_app_data *)data;

	Elm_Object_Item *nf_it = event_info;
	elm_genlist_item_selected_set(nf_it, EINA_FALSE);

	ad->modified_place = (myplace_data *) malloc(sizeof(myplace_data));
	if (ad->modified_place == NULL) return;

	ad->modified_place->method_map = true;
	ad->modified_place->latitude = ad->selected_place->latitude;
	ad->modified_place->longitude = ad->selected_place->longitude;
	ad->modified_place->address = NULL;

	if (ad->selected_place->address != NULL)
		ad->modified_place->address = strdup(ad->selected_place->address);

	mapview(ad, ad->modified_place);
}

static void select_wifi_method_cb(void *data, Evas_Object *obj, void *event_info)
{
	LS_FUNC_ENTER

	myplace_app_data *ad = (myplace_app_data *)data;

	Elm_Object_Item *nf_it = event_info;
	elm_genlist_item_selected_set(nf_it, EINA_FALSE);

	app_control_h app_control = NULL;
	app_control_create(&app_control);

	app_control_set_app_id(app_control, "wifi-efl-ug");
	app_control_add_extra_data(app_control, "caller", "lbhome");

	app_control_send_launch_request(app_control, service_wifi_reply_cb, ad);

	app_control_destroy(app_control);
}

static void select_bt_method_cb(void *data, Evas_Object *obj, void *event_info)
{
	LS_FUNC_ENTER

	myplace_app_data *ad = (myplace_app_data *)data;

	Elm_Object_Item *nf_it = event_info;
	elm_genlist_item_selected_set(nf_it, EINA_FALSE);

	app_control_h app_control = NULL;
	app_control_create(&app_control);

	app_control_set_app_id(app_control, "ug-bluetooth-efl");
	app_control_add_extra_data(app_control, "launch-type", "pick");
	app_control_send_launch_request(app_control, service_bluetooth_reply_cb, ad);

	app_control_destroy(app_control);
}

static Evas_Object *create_name_view(myplace_app_data *ad, Evas_Object* obj)
{
	Evas_Object *gen_list;
	Elm_Genlist_Item_Class *gen_name, *gen_description;
	Elm_Object_Item *gi_description;

	gen_list = elm_genlist_add(obj);
	elm_genlist_mode_set(gen_list, ELM_LIST_COMPRESS);
	elm_layout_theme_set(gen_list, "genlist", "base", "default");
	evas_object_data_set(gen_list, "app_data", ad);

	/* Place Name */
	gen_name = elm_genlist_item_class_new();
	if (gen_name == NULL)
		return NULL;
	gen_name->item_style = "entry.main";
	gen_name->func.text_get = myplace_place_name_text_get;
	gen_name->func.content_get = myplace_place_name_content_get;
	gen_name->func.state_get = NULL;
	gen_name->func.del = NULL;
	elm_genlist_item_append(gen_list, gen_name, NULL, NULL, ELM_GENLIST_ITEM_TREE, NULL, NULL);

	/* description */
	gen_description = elm_genlist_item_class_new();
	if (gen_description == NULL)
		return NULL;
	gen_description->item_style = "multiline_sub";
	gen_description->func.text_get = myplace_discription_text_get;
	gen_description->func.content_get = NULL;
	gen_description->func.state_get = NULL;
	gen_description->func.del = NULL;
	gi_description = elm_genlist_item_append(gen_list, gen_description, NULL, NULL, ELM_GENLIST_ITEM_NONE, NULL, NULL);

	elm_genlist_item_select_mode_set(gi_description, ELM_OBJECT_SELECT_MODE_NONE);

	evas_object_show(gen_list);

	elm_genlist_item_class_free(gen_name);
	elm_genlist_item_class_free(gen_description);

	return gen_list;
}

static Evas_Object *create_method_view(myplace_app_data *ad, Evas_Object* obj)
{
	Elm_Genlist_Item_Class *gen_fence_group, *gen_map_method = NULL, *gen_wifi_method, *gen_bt_method;

	ad->fence_genlist = elm_genlist_add(obj);
	elm_genlist_mode_set(ad->fence_genlist, ELM_LIST_COMPRESS);
	evas_object_data_set(ad->fence_genlist, "app_data", ad);

	/* fence Group */
	gen_fence_group = elm_genlist_item_class_new();
	if (gen_fence_group == NULL)
		return NULL;
	gen_fence_group->item_style = "groupindex";
	gen_fence_group->func.text_get = myplace_fence_group_text_get;
	gen_fence_group->func.content_get = NULL;
	gen_fence_group->func.state_get = NULL;
	gen_fence_group->func.del = NULL;
	elm_genlist_item_append(ad->fence_genlist, gen_fence_group, NULL, NULL, ELM_GENLIST_ITEM_NONE, NULL, NULL);

	if (ad->selected_place->place_id != PLACE_ID_CAR) {		/* Car doesn't have the Map as UX Guide */
		/* Map */
		gen_map_method = elm_genlist_item_class_new();
		if (gen_map_method == NULL)
			return NULL;
		gen_map_method->item_style = "2line.top";
		gen_map_method->func.text_get = myplace_select_map_text_get;
		gen_map_method->func.content_get = gl_button_content_get_cb;
		gen_map_method->func.state_get = NULL;
		gen_map_method->func.del = NULL;
		ad->gi_map_method = elm_genlist_item_append(ad->fence_genlist, gen_map_method, (void *)MYPLACE_METHOD_MAP, NULL, ELM_GENLIST_ITEM_TREE, select_map_method_cb, ad);
	}

	/* Wifi */
	gen_wifi_method = elm_genlist_item_class_new();
	if (gen_wifi_method == NULL)
		return NULL;
	gen_wifi_method->item_style = "2line.top";
	gen_wifi_method->func.text_get = myplace_select_wifi_text_get;
	gen_wifi_method->func.content_get = gl_button_content_get_cb;
	gen_wifi_method->func.state_get = NULL;
	gen_wifi_method->func.del = NULL;
	ad->gi_wifi_method = elm_genlist_item_append(ad->fence_genlist, gen_wifi_method, (void *)MYPLACE_METHOD_WIFI, NULL, ELM_GENLIST_ITEM_TREE, select_wifi_method_cb, ad);

	/* BT */
	gen_bt_method = elm_genlist_item_class_new();
	if (gen_bt_method == NULL)
		return NULL;
	gen_bt_method->item_style = "2line.top";
	gen_bt_method->func.text_get = myplace_select_bt_text_get;
	gen_bt_method->func.content_get = gl_button_content_get_cb;
	gen_bt_method->func.state_get = NULL;
	gen_bt_method->func.del = NULL;
	ad->gi_bt_method = elm_genlist_item_append(ad->fence_genlist, gen_bt_method, (void *)MYPLACE_METHOD_BT, NULL, ELM_GENLIST_ITEM_TREE, select_bt_method_cb, ad);

	evas_object_show(ad->fence_genlist);

	elm_genlist_item_class_free(gen_fence_group);
	if (ad->selected_place->place_id != PLACE_ID_CAR)		/* Car doesn't have the Map as UX Guide */
		elm_genlist_item_class_free(gen_map_method);

	elm_genlist_item_class_free(gen_wifi_method);
	elm_genlist_item_class_free(gen_bt_method);

	return ad->fence_genlist;
}


static Evas_Object *create_detail_view(myplace_app_data *ad)
{
	Evas_Object *main_scroller, *main_box;

	main_scroller = elm_scroller_add(ad->nf);
	elm_scroller_bounce_set(main_scroller, EINA_FALSE, EINA_TRUE);
	evas_object_size_hint_weight_set(main_scroller, EVAS_HINT_EXPAND, 0.0);
	evas_object_size_hint_align_set(main_scroller, EVAS_HINT_FILL, 0.0);

	evas_object_show(main_scroller);

	main_box = elm_box_add(main_scroller);
	evas_object_size_hint_align_set(main_box, EVAS_HINT_FILL, 0.0);
	evas_object_size_hint_weight_set(main_box, EVAS_HINT_EXPAND, 0.0);
	evas_object_show(main_box);

	Evas_Object *name_layout = create_name_view(ad, main_box);
	evas_object_size_hint_weight_set(name_layout, EVAS_HINT_EXPAND, 0.45);
	evas_object_size_hint_align_set(name_layout, EVAS_HINT_FILL, EVAS_HINT_FILL);
	evas_object_show(name_layout);
	elm_box_pack_end(main_box, name_layout);

	Evas_Object *method_layout = create_method_view(ad, main_box);
	evas_object_size_hint_weight_set(method_layout, EVAS_HINT_EXPAND, 0.55);
	evas_object_size_hint_align_set(method_layout, EVAS_HINT_FILL, EVAS_HINT_FILL);
	evas_object_show(method_layout);
	elm_box_pack_end(main_box, method_layout);

	elm_object_content_set(main_scroller, main_box);

	return main_scroller;
}

void placeinfo_cb(void *data, Evas_Object *obj, void *event_info)
{
	LS_FUNC_ENTER

	myplace_app_data *ad = evas_object_data_get(obj, "app_data");
	int index = (int)data;

	Evas_Object *cancel_btn, *done_btn;
	Elm_Object_Item *nf_it = NULL;

	if (ad == NULL) return;

	if (ad->ctx_popup) {
		evas_object_del(ad->ctx_popup);
		ad->ctx_popup = NULL;
	} else {
		nf_it = event_info;
		elm_genlist_item_selected_set(nf_it, EINA_FALSE);
	}

	if (index >= MAX_PLACE_COUNT) {
		toast_popup(ad, P_("IDS_COM_BODY_UNABLE_TO_ADD"));
		return;
	}

	ad->selected_place = (myplace_data *) malloc(sizeof(myplace_data));
	if (ad->selected_place == NULL) return;

	ad->selected_place->name = NULL;
	ad->selected_place->is_default = false;

	ad->selected_place->place_id = -1;
	ad->selected_place->map_fence_id = -1;
	ad->selected_place->wifi_fence_id = -1;
	ad->selected_place->bt_fence_id = -1;

	ad->selected_place->latitude = -1;
	ad->selected_place->longitude = -1;
	ad->selected_place->address = NULL;
	ad->selected_place->wifi_bssid = NULL;
	ad->selected_place->wifi_ssid = NULL;
	ad->selected_place->bt_mac_address = NULL;
	ad->selected_place->bt_ssid = NULL;

	ad->selected_place->map_geofence_params = NULL;
	ad->selected_place->wifi_geofence_params = NULL;
	ad->selected_place->bt_geofence_params = NULL;

	ad->selected_place->method_map = false;
	ad->selected_place->method_wifi = false;
	ad->selected_place->method_bt = false;

	if (index <= ad->last_index) {
		/* keep original data */
		ad->selected_place->name = strdup(ad->placelist[index]->name);
		ad->selected_place->is_default = ad->placelist[index]->is_default;

		ad->selected_place->place_id = ad->placelist[index]->place_id;
		ad->selected_place->map_fence_id = ad->placelist[index]->map_fence_id;
		ad->selected_place->wifi_fence_id = ad->placelist[index]->wifi_fence_id;
		ad->selected_place->bt_fence_id = ad->placelist[index]->bt_fence_id;

		if (ad->placelist[index]->method_map == true) {
			ad->selected_place->method_map = ad->placelist[index]->method_map;
			ad->selected_place->latitude = ad->placelist[index]->latitude;
			ad->selected_place->longitude = ad->placelist[index]->longitude;
			ad->selected_place->address = strdup(ad->placelist[index]->address);
			ad->selected_place->map_geofence_params = ad->placelist[index]->map_geofence_params;
		}
		if (ad->placelist[index]->method_wifi == true) {
			ad->selected_place->method_wifi = ad->placelist[index]->method_wifi;
			ad->selected_place->wifi_bssid = strdup(ad->placelist[index]->wifi_bssid);
			ad->selected_place->wifi_ssid = strdup(ad->placelist[index]->wifi_ssid);
			ad->selected_place->wifi_geofence_params = ad->placelist[index]->wifi_geofence_params;
		}
		if (ad->placelist[index]->method_bt == true) {
			ad->selected_place->method_bt = ad->placelist[index]->method_bt;
			ad->selected_place->bt_mac_address = strdup(ad->placelist[index]->bt_mac_address);
			ad->selected_place->bt_ssid = strdup(ad->placelist[index]->bt_ssid);
			ad->selected_place->bt_geofence_params = ad->placelist[index]->bt_geofence_params;
		}
	}

	ad->current_index = index;

	Evas_Object *layout = create_detail_view(ad);

	if (index > ad->last_index)
		nf_it = elm_naviframe_item_push(ad->nf, P_("IDS_ST_HEADER_ADD_PLACE_ABB"), NULL, NULL, layout, NULL);
	else
		nf_it = elm_naviframe_item_push(ad->nf, P_("IDS_ST_HEADER_EDIT_PLACE_ABB"), NULL, NULL, layout, NULL);

	/* title cancel button */
	cancel_btn = elm_button_add(ad->nf);
	/* elm_object_style_set(cancel_btn, "naviframe/title_text_left"); */
	elm_object_style_set(cancel_btn, "naviframe/title_text");
	elm_object_part_text_set(cancel_btn, "default", P_("IDS_COM_SK_CANCEL"));
	evas_object_smart_callback_add(cancel_btn, "clicked", detailinfo_cancel_cb, ad);
	/* elm_object_item_part_content_set(nf_it, "title_left_text_btn", cancel_btn); */
	elm_object_item_part_content_set(nf_it, "title_left_btn", cancel_btn);

	/* title done button */
	done_btn = elm_button_add(ad->nf);
	/* elm_object_style_set(done_btn, "naviframe/title_text_right"); */
	elm_object_style_set(done_btn, "naviframe/title_text");
	elm_object_part_text_set(done_btn, "default", P_("IDS_COM_BODY_DONE"));
	evas_object_smart_callback_add(done_btn, "clicked", detailinfo_done_cb, ad);
	/* elm_object_item_part_content_set(nf_it, "title_right_text_btn", done_btn); */
	elm_object_item_part_content_set(nf_it, "title_right_btn", done_btn);
}
