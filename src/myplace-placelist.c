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
#include "myplace-elementary.h"
#include "myplace-detailinfo.h"
#include "myplace-placelist.h"
#include "myplace-delete.h"
#include "myplace.h"

Evas_Object *create_indicator_bg(Evas_Object * parent)
{
	LS_FUNC_ENTER
	Evas_Object *indicator_bg = NULL;

	indicator_bg = elm_bg_add(parent);
	elm_object_style_set(indicator_bg, "indicator/headerbg");
	elm_object_part_content_set(parent, "elm.swallow.indicator_bg", indicator_bg);
	evas_object_show(indicator_bg);

	LS_FUNC_EXIT
	return indicator_bg;
	}

Evas_Object *create_bg(Evas_Object * parent)
{
	LS_FUNC_ENTER
	Evas_Object *bg = elm_bg_add(parent);
	LS_RETURN_VAL_IF_FAILED(bg, NULL);
	evas_object_size_hint_weight_set(bg, EVAS_HINT_EXPAND, EVAS_HINT_EXPAND);
	elm_win_resize_object_add(parent, bg);
	evas_object_show(bg);
	LS_FUNC_EXIT
	return bg;
}

Evas_Object *create_conformant(Evas_Object * parent)
{
	LS_FUNC_ENTER
	Evas_Object *conformant = NULL;

	conformant = elm_conformant_add(parent);
	evas_object_size_hint_weight_set(conformant, EVAS_HINT_EXPAND, EVAS_HINT_EXPAND);
	elm_win_resize_object_add(parent, conformant);
	evas_object_show(conformant);

	LS_FUNC_EXIT
	return conformant;
}

Evas_Object *create_layout(Evas_Object * parent)
{
	LS_FUNC_ENTER
	Evas_Object *layout = NULL;

	if (parent != NULL) {
		layout = elm_layout_add(parent);
		evas_object_size_hint_weight_set(layout, EVAS_HINT_EXPAND, EVAS_HINT_EXPAND);
		elm_layout_theme_set(layout, "layout", "application", "default");
		elm_object_content_set(parent, layout);
		evas_object_show(layout);
	}

	LS_FUNC_EXIT
	return layout;
}

void profile_changed_cb(void *data, Evas_Object * obj, void *event)
{
	LS_FUNC_ENTER
	const char *profile = elm_config_profile_get();

	if (strcmp(profile, "desktop") == 0)
		elm_win_indicator_mode_set(obj, ELM_WIN_INDICATOR_HIDE);
	else
		elm_win_indicator_mode_set(obj, ELM_WIN_INDICATOR_SHOW);

	LS_FUNC_EXIT
}

void win_del(void *data, Evas_Object * obj, void *event)
{
	LS_FUNC_ENTER
	elm_exit();
}

Evas_Object *create_win(const char *name)
{
	LS_FUNC_ENTER
	Evas_Object *eo;
	eo = elm_win_util_standard_add(name, name);

	if (eo) {
		elm_win_title_set(eo, name);
		elm_win_indicator_mode_set(eo, ELM_WIN_INDICATOR_SHOW); /* indicator allow */
		elm_win_indicator_opacity_set(eo, ELM_WIN_INDICATOR_OPAQUE);
		/* elm_win_wm_desktop_layout_support_set(eo, EINA_TRUE); */		/* block for 3.0 build */
		elm_win_conformant_set(eo, EINA_TRUE);
		evas_object_smart_callback_add(eo, "delete,request", win_del, NULL);
		elm_win_autodel_set(eo, EINA_TRUE);
		if (elm_win_wm_rotation_supported_get(eo)) {
			int rots[4] = { 0, 90, 180, 270 };
			elm_win_wm_rotation_available_rotations_set(eo, (const int *)(&rots), 4);
		}
	}
	evas_object_show(eo);

	return eo;
}

static void _ctx_popup_dismissed_cb(void *data, Evas_Object *obj, void *event_info)
{
	myplace_app_data *ad = (myplace_app_data *)data;

	if (!ad->ctx_popup) {
		LS_LOGE("Invalid parameters");
		return;
	}

	evas_object_del(ad->ctx_popup);
	ad->ctx_popup = NULL;
}

static void _move_more_ctxpopup(void *data)
{
	LS_FUNC_ENTER

	myplace_app_data *ad = (myplace_app_data *)data;

	Evas_Coord w, h;
	int pos = -1;

	elm_win_screen_size_get(ad->win_main, NULL, NULL, &w, &h);
	pos = elm_win_rotation_get(ad->win_main);

	switch (pos) {
		case 0:
		case 180:
			evas_object_move(ad->ctx_popup, w, h);
			break;
		case 90:
			evas_object_move(ad->ctx_popup, h/2, w);
			break;
		case 270:
			evas_object_move(ad->ctx_popup, h/2, w);
			break;
	}
}

static void _rotate_more_ctxpopup_cb(void *data, Evas_Object *obj, void *event_info)
{
	LS_FUNC_ENTER
	myplace_app_data *ad = (myplace_app_data *)data;
	_move_more_ctxpopup(ad);
}

static void _resize_more_ctxpopup_cb(void *data, Evas *e, Evas_Object *obj, void *event_info)
{
	LS_FUNC_ENTER
	myplace_app_data *ad = (myplace_app_data *)data;
	_move_more_ctxpopup(ad);
}

/*
static int __myplace_geofence_set_int(const char *path, int val)
{
	int ret = vconf_set_int(path, val);
	if (ret == 0)
		return -1;

	return 0;
}

static void __myplace_geofence_set_disabled(myplace_app_data *ad)
{
	LS_RETURN_IF_FAILED(ad);

	ad->is_geofence = KEY_DISABLED;

	__myplace_geofence_set_int(VCONFKEY_LOCATION_GEOFENCE_ENABLED, KEY_DISABLED);
}

static void __myplace_geofence_set_enabled(myplace_app_data *ad)
{
	LS_RETURN_IF_FAILED(ad);

	ad->is_geofence = KEY_ENABLED;

	__myplace_geofence_set_int(VCONFKEY_LOCATION_GEOFENCE_ENABLED, KEY_ENABLED);
}
*/

static bool myplace_fence_cb(int geofence_id, geofence_h fence, int fence_index, int fence_cnt, void *user_data)
{
	LS_FUNC_ENTER

	myplace_app_data *ad = (myplace_app_data *)user_data;

	double latitude = 0.0, longitude = 0.0;
	char *address = NULL;
	char *wifi_bssid = NULL, *wifi_ssid = NULL;
	char *bt_mac_address = NULL, *bt_ssid = NULL;
	geofence_type_e type = 0;

	LS_LOGE("fence_id: %d", geofence_id);

	if (fence) {
		geofence_get_type(fence, &type);

		switch (type) {
		case MYPLACE_METHOD_MAP:
			ad->placelist[ad->current_index]->map_fence_id = geofence_id;
			ad->placelist[ad->current_index]->method_map = true;
			ad->placelist[ad->current_index]->map_geofence_params = fence;
			geofence_get_latitude(fence, &latitude);
			ad->placelist[ad->current_index]->latitude = latitude;
			geofence_get_longitude(fence, &longitude);
			ad->placelist[ad->current_index]->longitude = longitude;
			geofence_get_address(fence, &address);
			ad->placelist[ad->current_index]->address = strdup(address);
			break;
		case MYPLACE_METHOD_WIFI:
			ad->placelist[ad->current_index]->wifi_fence_id = geofence_id;
			ad->placelist[ad->current_index]->method_wifi = true;
			ad->placelist[ad->current_index]->wifi_geofence_params = fence;
			geofence_get_bssid(fence, &wifi_bssid);
			ad->placelist[ad->current_index]->wifi_bssid = strdup(wifi_bssid);
			geofence_get_ssid(fence, &wifi_ssid);
			ad->placelist[ad->current_index]->wifi_ssid = strdup(wifi_ssid);
			break;
		case MYPLACE_METHOD_BT:
			ad->placelist[ad->current_index]->bt_fence_id = geofence_id;
			ad->placelist[ad->current_index]->method_bt = true;
			ad->placelist[ad->current_index]->bt_geofence_params = fence;
			geofence_get_bssid(fence, &bt_mac_address);
			ad->placelist[ad->current_index]->bt_mac_address = strdup(bt_mac_address);
			geofence_get_ssid(fence, &bt_ssid);
			ad->placelist[ad->current_index]->bt_ssid = strdup(bt_ssid);
			break;
		default:
			break;
		}
	}

	if (address != NULL) {
		free(address);
		address = NULL;
	}
	if (wifi_bssid != NULL) {
		free(wifi_bssid);
		wifi_bssid = NULL;
	}
	if (wifi_ssid != NULL) {
		free(wifi_ssid);
		wifi_ssid = NULL;
	}
	if (bt_mac_address != NULL) {
		free(bt_mac_address);
		bt_mac_address = NULL;
	}
	if (bt_ssid != NULL) {
		free(bt_ssid);
		bt_ssid = NULL;
	}
	return true;
}

static bool myplace_place_cb(int place_id, const char *place_name, int place_index, int place_cnt, void *user_data)
{
	LS_FUNC_ENTER

	myplace_app_data *ad = (myplace_app_data *)user_data;
	int error = GEOFENCE_MANAGER_ERROR_NONE;

	LS_LOGE("place_id: %d", place_id);

	ad->last_index++;
	ad->current_index = ad->last_index;

	/* init new place */
	ad->placelist[ad->last_index] = (myplace_data *) malloc(sizeof(myplace_data));

	ad->placelist[ad->last_index]->latitude = -1;
	ad->placelist[ad->last_index]->longitude = -1;
	ad->placelist[ad->last_index]->address = NULL;
	ad->placelist[ad->last_index]->wifi_bssid = NULL;
	ad->placelist[ad->last_index]->wifi_ssid = NULL;
	ad->placelist[ad->last_index]->bt_mac_address = NULL;
	ad->placelist[ad->last_index]->bt_ssid = NULL;

	ad->placelist[ad->last_index]->map_geofence_params = NULL;
	ad->placelist[ad->last_index]->wifi_geofence_params = NULL;
	ad->placelist[ad->last_index]->bt_geofence_params = NULL;

	ad->placelist[ad->last_index]->method_map = false;
	ad->placelist[ad->last_index]->method_wifi = false;
	ad->placelist[ad->last_index]->method_bt = false;

	if (ad->last_index >= DEFAULT_PLACE_COUNT)
		ad->placelist[ad->last_index]->is_default = false;
	else
		ad->placelist[ad->last_index]->is_default = true;

	ad->placelist[ad->last_index]->place_id = place_id;
	ad->placelist[ad->last_index]->name = strdup(place_name);

	error = geofence_manager_foreach_place_geofence_list(ad->geo_manager, ad->placelist[ad->last_index]->place_id, myplace_fence_cb, ad);

	if (error != GEOFENCE_MANAGER_ERROR_NONE)
		LS_LOGE("geofence_manager_foreach_fence_list FAILED: %d", error);

	return true;
}

int myplace_geofence_init(myplace_app_data *ad)
{
	LS_RETURN_VAL_IF_FAILED(ad, -1);

	int ret = 0;
	int error = GEOFENCE_MANAGER_ERROR_NONE;

	ad->last_index = -1;
	ad->current_index = -1;

	/* ret &= vconf_get_int(VCONFKEY_LOCATION_GEOFENCE_ENABLED, &ad->is_geofence); */
	/* ret &= vconf_notify_key_changed(VCONFKEY_LOCATION_GEOFENCE_ENABLED, _gps_enabled_cb, (void *)ad); */

	if (ad->geo_manager == NULL)
		error = geofence_manager_create(&(ad->geo_manager));

	if (error == GEOFENCE_MANAGER_ERROR_NONE) {
		error = geofence_manager_foreach_place_list(ad->geo_manager, myplace_place_cb, ad);

		if (error != GEOFENCE_MANAGER_ERROR_NONE)
			LS_LOGE("geofence_manager_foreach_place_list FAILED: %d\n", error);

	} else
		LS_LOGE("geofence_manager_create FAILED %d", error);

	return ret;
}

int myplace_geofence_deinit(myplace_app_data *ad)
{
	int ret = 0;

	/* ret = vconf_ignore_key_changed(VCONFKEY_LOCATION_GEOFENCE_ENABLED, _gps_enabled_cb); */

	return ret;
}

static char *myplace_discription_text_get(void *data, Evas_Object *obj, const char *part)
{
	if (!g_strcmp0(part, "elm.text.multiline"))
		return strdup(P_("IDS_ST_BODY_SAVE_YOUR_FAVOURITE_LOCATIONS_FOR_USE_WITH_APPLICATIONS_THAT_REQUIRE_LOCATION_INFORMATION"));

	return NULL;
}

char *myplace_place_text_get(void *data, Evas_Object *obj, const char *part)
{
	long int index = (long int) data;
	myplace_app_data *ad = evas_object_data_get(obj, "app_data");
	char geo_method[50] = {};

	if (ad == NULL)
		return NULL;

	if (!g_strcmp0(part, "elm.text"))
		return strdup(ad->placelist[index]->name);

	if (!g_strcmp0(part, "elm.text.sub")) {
		if (ad->placelist[index]->method_map == true) {
			if (ad->placelist[index]->method_wifi == true) {
				if (ad->placelist[index]->method_bt == true)
					g_snprintf(geo_method, sizeof(geo_method), "%s,%s,%s", P_("IDS_LBS_BODY_MAP"), P_("IDS_COM_BODY_WI_FI"), P_("IDS_COM_BODY_BLUETOOTH"));
				else
					g_snprintf(geo_method, sizeof(geo_method), "%s,%s", P_("IDS_LBS_BODY_MAP"), P_("IDS_COM_BODY_WI_FI"));
			} else {
				if (ad->placelist[index]->method_bt == true)
					g_snprintf(geo_method, sizeof(geo_method), "%s,%s", P_("IDS_LBS_BODY_MAP"), P_("IDS_COM_BODY_BLUETOOTH"));
				else
					g_snprintf(geo_method, sizeof(geo_method), "%s", P_("IDS_LBS_BODY_MAP"));
			}
		} else {
			if (ad->placelist[index]->method_wifi == true) {
				if (ad->placelist[index]->method_bt == true)
					g_snprintf(geo_method, sizeof(geo_method), "%s,%s", P_("IDS_COM_BODY_WI_FI"), P_("IDS_COM_BODY_BLUETOOTH"));
				else
					g_snprintf(geo_method, sizeof(geo_method), "%s", P_("IDS_COM_BODY_WI_FI"));
			} else {
				if (ad->placelist[index]->method_bt == true)
					g_snprintf(geo_method, sizeof(geo_method), "%s", P_("IDS_COM_BODY_BLUETOOTH"));
				else
					return strdup(P_("IDS_COM_BODY_NONE"));
			}
		}
	}

	return strdup(geo_method);
}

static Evas_Object *myplace_placelist_create_gl(Evas_Object *parent, void *data)
{
	LS_FUNC_ENTER

	LS_RETURN_VAL_IF_FAILED(data, NULL);
	LS_RETURN_VAL_IF_FAILED(parent, NULL);

	myplace_app_data *ad = (myplace_app_data *)data;
	Evas_Object *genlist = NULL;
	Elm_Genlist_Item_Class *itc_discription;
	Elm_Object_Item *gi_discription;

	long int i;

	genlist = elm_genlist_add(parent);
	elm_genlist_mode_set(genlist, ELM_LIST_COMPRESS);

	ad->current_index = 0;
	evas_object_data_set(genlist, "app_data", ad);

	/* discription */
	itc_discription = elm_genlist_item_class_new();
	if (itc_discription == NULL)
		return NULL;
	itc_discription->item_style = "multiline";
	itc_discription->func.text_get = myplace_discription_text_get;
	itc_discription->func.content_get = NULL;
	itc_discription->func.state_get = NULL;
	itc_discription->func.del = NULL;
	gi_discription = elm_genlist_item_append(genlist, itc_discription, NULL, NULL, ELM_GENLIST_ITEM_NONE, NULL, NULL);

	elm_genlist_item_select_mode_set(gi_discription, ELM_OBJECT_SELECT_MODE_NONE);

	for (i = 0; i <= ad->last_index; i++) {
		/* add more place to genlist */
		ad->placelist[i]->itc_myplace = elm_genlist_item_class_new();
		if (ad->placelist[i]->itc_myplace == NULL)
			return NULL;
		ad->placelist[i]->itc_myplace->item_style = "type1";
		ad->placelist[i]->itc_myplace->func.text_get = myplace_place_text_get;
		ad->placelist[i]->itc_myplace->func.content_get = NULL;
		ad->placelist[i]->itc_myplace->func.state_get = NULL;
		ad->placelist[i]->itc_myplace->func.del = NULL;
		ad->placelist[i]->gi_myplace = elm_genlist_item_append(genlist, ad->placelist[i]->itc_myplace, (void *)i, NULL, ELM_GENLIST_ITEM_NONE, placeinfo_cb, (void *)i);
	}
	return genlist;
}

static void myplace_more_button(void *data, Evas_Object *obj, void *event_info)
{
	LS_RETURN_IF_FAILED(data);
	myplace_app_data *ad = (myplace_app_data *)data;

	if (!ad || !ad->nf) {
		LS_LOGE("NULL parameters");
		return;
	}

	if (ad->ctx_popup) {
		evas_object_del(ad->ctx_popup);
		ad->ctx_popup = NULL;
	}

	ad->ctx_popup = elm_ctxpopup_add(ad->win_main);
	elm_object_style_set(ad->ctx_popup, "more/default");
	eext_object_event_callback_add(ad->ctx_popup, EEXT_CALLBACK_BACK, eext_ctxpopup_back_cb, NULL);
	eext_object_event_callback_add(ad->ctx_popup, EEXT_CALLBACK_MORE, eext_ctxpopup_back_cb, NULL);
	evas_object_smart_callback_add(ad->ctx_popup, "dismissed", _ctx_popup_dismissed_cb, ad);
	elm_ctxpopup_auto_hide_disabled_set(ad->ctx_popup, EINA_TRUE);

	evas_object_event_callback_add(ad->nf, EVAS_CALLBACK_RESIZE, _resize_more_ctxpopup_cb, ad);
	evas_object_smart_callback_add(elm_object_top_widget_get(ad->ctx_popup), "rotation,changed", _rotate_more_ctxpopup_cb, ad);

	evas_object_data_set(ad->ctx_popup, "app_data", ad);

	elm_ctxpopup_item_append(ad->ctx_popup, P_("IDS_FM_OPT_ADD"), NULL, placeinfo_cb, (void *)(ad->last_index + 1));
	elm_ctxpopup_item_append(ad->ctx_popup, P_("IDS_CCL_OPT_DELETE"), NULL, myplace_delete_cb, ad);

	elm_ctxpopup_direction_priority_set(ad->ctx_popup, ELM_CTXPOPUP_DIRECTION_UP, ELM_CTXPOPUP_DIRECTION_LEFT, ELM_CTXPOPUP_DIRECTION_RIGHT, ELM_CTXPOPUP_DIRECTION_DOWN);

    _move_more_ctxpopup(ad);

	evas_object_show(ad->ctx_popup);
}

static Evas_Object *__myplace_create_navibar(Evas_Object *parent)
{
	LS_RETURN_VAL_IF_FAILED(parent, NULL);
	Evas_Object *naviframe = NULL;

	naviframe = elm_naviframe_add(parent);
	elm_object_part_content_set(parent, "elm.swallow.content", naviframe);
	evas_object_show(naviframe);

	return naviframe;
}

static Eina_Bool __myplace_pop_cb(void *data, Elm_Object_Item *item)
{
	LS_FUNC_ENTER
	elm_exit();
	return EINA_FALSE;
}

void myplace_placelist_cb(void *data)
{
	LS_FUNC_ENTER

	myplace_app_data *ad = (myplace_app_data *)data;
	int i;

	Evas_Object *back_button, *more_button = NULL;
	Elm_Object_Item *nf_it;

	ad->nf = __myplace_create_navibar(ad->layout_main);
	elm_naviframe_prev_btn_auto_pushed_set(ad->nf, EINA_FALSE);
	eext_object_event_callback_add(ad->nf, EEXT_CALLBACK_BACK, eext_naviframe_back_cb, NULL);
	eext_object_event_callback_add(ad->nf, EEXT_CALLBACK_MORE, eext_naviframe_more_cb, NULL);

	/* genList */
	ad->place_genlist = myplace_placelist_create_gl(ad->nf, ad);
	evas_object_show(ad->place_genlist);

	back_button = elm_button_add(ad->nf);
	elm_object_style_set(back_button, "naviframe/back_btn/default");
	evas_object_smart_callback_add(back_button, "clicked", _myplace_win_quit_cb, ad);

	nf_it = elm_naviframe_item_push(ad->nf, P_("IDS_MAPS_BODY_MY_PLACES"), back_button, NULL, ad->place_genlist, NULL);

	more_button = elm_button_add(ad->nf);
	elm_object_style_set(more_button, "naviframe/more/default");
	evas_object_smart_callback_add(more_button, "clicked", myplace_more_button, ad);
	elm_object_item_part_content_set(nf_it, "toolbar_more_btn", more_button);

	for (i = 0; ad->last_index >= i; i++)
		elm_genlist_item_class_free(ad->placelist[i]->itc_myplace);

	elm_naviframe_item_pop_cb_set(nf_it, __myplace_pop_cb, ad);
}
