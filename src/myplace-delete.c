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

static void __update_title(myplace_app_data *ad)
{
	char delete_title[20] = { 0, };
	char tmp_buf[20] = { 0, };

	int del_cnt = 0, i = 0;

	for (i = DEFAULT_PLACE_COUNT; i <= ad->last_index; i++) {
		if (ad->placelist[i]->del_check == EINA_TRUE)
			del_cnt++;
	}

	snprintf(tmp_buf, sizeof(tmp_buf), "%s", P_("IDS_ST_HEADER_PD_SELECTED_ABB"));
	snprintf(delete_title, 20, tmp_buf, del_cnt);

	/* elm_object_item_domain_text_translatable_set(ad->nf_it, PACKAGE, EINA_TRUE); */
	elm_object_item_part_text_set(ad->nf_it, "default", delete_title);
}

static char *delete_name_text_get(void *data, Evas_Object *obj, const char *part)
{
	long int index = (long int)data;
	myplace_app_data *ad = evas_object_data_get(obj, "app_data");

	if (ad == NULL)
		return NULL;

	if (!g_strcmp0(part, "elm.text")) {
		if (index < 0)
			return strdup(P_("IDS_COM_BODY_SELECT_ALL"));
		else
			return strdup(ad->placelist[index]->name);
	}
	return NULL;
}

static Eina_Bool get_check_all(myplace_app_data *ad)
{
	Eina_Bool sel_all = EINA_TRUE;
	int i;

	if (ad->last_index + 1 == DEFAULT_PLACE_COUNT)
		return EINA_FALSE;

	for (i = DEFAULT_PLACE_COUNT; i <= ad->last_index; i++) {
		if (ad->placelist[i]->del_check == EINA_FALSE)
			sel_all = EINA_FALSE;
	}

	return sel_all;
}

static void delete_all_check_cb(void *data, Evas_Object *obj, void *event_info)
{
	myplace_app_data *ad = evas_object_data_get(obj, "app_data");
	Evas_Object *check = obj;

	Eina_Bool sel_all = elm_check_state_get(check);

	int i;

	if (ad == NULL)
		return;

	if (sel_all == EINA_TRUE) {
		for (i = DEFAULT_PLACE_COUNT; i <= ad->last_index; i++)
			ad->placelist[i]->del_check = EINA_TRUE;
	} else {
		for (i = DEFAULT_PLACE_COUNT; i <= ad->last_index; i++)
			ad->placelist[i]->del_check = EINA_FALSE;
	}

	for (i = DEFAULT_PLACE_COUNT; i <= ad->last_index; i++)
		elm_genlist_item_update(ad->placelist[i]->gi_delplace);

	__update_title(ad);
}

static Evas_Object *delete_all_check_get(void *data, Evas_Object *obj, const char *part)
{
	myplace_app_data *ad = evas_object_data_get(obj, "app_data");
	Evas_Object *all_check = evas_object_data_get(obj, "all_check");

	Eina_Bool sel_all;

	if (ad == NULL)
		return NULL;

	if (!strcmp(part, "elm.swallow.icon.1")) {
		all_check = elm_check_add(obj);

		sel_all = get_check_all(ad);

		evas_object_propagate_events_set(all_check, EINA_FALSE);
		elm_check_state_set(all_check, sel_all);

		evas_object_data_set(all_check, "app_data", ad);
		evas_object_smart_callback_add(all_check, "changed", delete_all_check_cb, NULL);

		evas_object_show(all_check);

		return all_check;
	}
	return NULL;
}

static void set_checkall_by_genlist_cb(void *data, Evas_Object *obj, void *event_info)
{
	myplace_app_data *ad = evas_object_data_get(obj, "app_data");
	Evas_Object *all_check = evas_object_data_get(obj, "all_check");

	Eina_Bool sel_all = NULL;
	int i;

	Elm_Object_Item *nf_it = event_info;
	elm_genlist_item_selected_set(nf_it, EINA_FALSE);

	if (ad == NULL)
		return;

	sel_all = get_check_all(ad);

	if (sel_all == EINA_FALSE) {
		for (i = DEFAULT_PLACE_COUNT; i <= ad->last_index; i++)
			ad->placelist[i]->del_check = EINA_TRUE;
	} else {
		for (i = DEFAULT_PLACE_COUNT; i <= ad->last_index; i++)
			ad->placelist[i]->del_check = EINA_FALSE;
	}

	for (i = DEFAULT_PLACE_COUNT; i <= ad->last_index; i++)
		elm_genlist_item_update(ad->placelist[i]->gi_delplace);

	elm_check_state_set(all_check, get_check_all(ad));
	elm_genlist_item_update(ad->gi_del_all);

	__update_title(ad);
}

static void set_check_by_genlist_cb(void *data, Evas_Object *obj, void *event_info)
{
	myplace_app_data *ad = evas_object_data_get(obj, "app_data");
	Evas_Object *all_check = evas_object_data_get(obj, "all_check");
	long int index = (long int)data;

	Elm_Object_Item *nf_it = event_info;
	elm_genlist_item_selected_set(nf_it, EINA_FALSE);

	if (ad == NULL)
		return;

	if (ad->placelist[index]->del_check == EINA_TRUE)
		ad->placelist[index]->del_check = EINA_FALSE;
	else
		ad->placelist[index]->del_check = EINA_TRUE;

	elm_genlist_item_update(ad->placelist[index]->gi_delplace);

	elm_check_state_set(all_check, get_check_all(ad));
	elm_genlist_item_update(ad->gi_del_all);

	__update_title(ad);
}

static void set_check_cb(void *data, Evas_Object *obj, void *event_info)
{
	Evas_Object *all_check = evas_object_data_get(obj, "all_check");
	myplace_app_data *ad = evas_object_data_get(obj, "app_data");
	long int index = (long int)data;

	if (ad == NULL)
		return;

	if (ad->placelist[index]->del_check == EINA_TRUE)
		ad->placelist[index]->del_check = EINA_FALSE;
	else
		ad->placelist[index]->del_check = EINA_TRUE;

	elm_check_state_set(all_check, get_check_all(ad));
	elm_genlist_item_update(ad->gi_del_all);

	__update_title(ad);
}

static Evas_Object *delete_check_get(void *data, Evas_Object *obj, const char *part)
{
	myplace_app_data *ad = evas_object_data_get(obj, "app_data");
	long int index = (long int)data;

	Evas_Object *check = evas_object_data_get(obj, "check");
	Evas_Object *all_check = evas_object_data_get(obj, "all_check");

	if (ad == NULL)
		return NULL;

	if (!strcmp(part, "elm.swallow.icon.1")) {
		check = elm_check_add(obj);

		elm_check_state_set(check, ad->placelist[index]->del_check);
		evas_object_propagate_events_set(check, EINA_FALSE);

		evas_object_data_set(check, "app_data", ad);
		evas_object_data_set(check, "all_check", all_check);
		evas_object_smart_callback_add(check, "changed", set_check_cb, (void *)index);

		evas_object_show(check);

		return check;
	}
	return NULL;
}

static void delete_cancel_cb(void *data, Evas_Object *obj, void *event_info)
{
	myplace_app_data *ad = (myplace_app_data *)data;
	elm_naviframe_item_pop(ad->nf);
}

static void delete_done_cb(void *data, Evas_Object *obj, void *event_info)
{
	myplace_app_data *ad = (myplace_app_data *)data;
	int i, j;
	int backup_last = ad->last_index;
	bool select_item = false;

	for (i = DEFAULT_PLACE_COUNT; i <= ad->last_index; i++) {
		if (ad->placelist[i]->del_check == EINA_TRUE) {
			select_item = true;

			if (ad->placelist[i]->method_map == true) {
				geofence_manager_remove_fence(ad->geo_manager, ad->placelist[i]->map_fence_id);
				geofence_destroy(ad->placelist[i]->map_geofence_params);
			}

			if (ad->placelist[i]->method_wifi == true) {
				geofence_manager_remove_fence(ad->geo_manager, ad->placelist[i]->map_fence_id);
				geofence_destroy(ad->placelist[i]->map_geofence_params);
			}

			if (ad->placelist[i]->method_bt == true) {
				geofence_manager_remove_fence(ad->geo_manager, ad->placelist[i]->map_fence_id);
				geofence_destroy(ad->placelist[i]->map_geofence_params);
			}

			geofence_manager_remove_place(ad->geo_manager, ad->placelist[i]->place_id);

			elm_object_item_del(ad->placelist[i]->gi_myplace);

			if (ad->placelist[i]->name)
				free(ad->placelist[i]->name);
			if (ad->placelist[i]->wifi_bssid != NULL)
				free(ad->placelist[i]->wifi_bssid);
			if (ad->placelist[i]->wifi_ssid != NULL)
				free(ad->placelist[i]->wifi_ssid);
			if (ad->placelist[i]->bt_mac_address != NULL)
				free(ad->placelist[i]->bt_mac_address);
			if (ad->placelist[i]->bt_ssid != NULL)
				free(ad->placelist[i]->bt_ssid);

			free(ad->placelist[i]);
			ad->placelist[i] = NULL;
		}
	}

	if (select_item == false) {
		toast_popup(ad, P_("IDS_BT_BODY_NOT_SELECTED"));
		return;
	}

	for (i = DEFAULT_PLACE_COUNT; i <= ad->last_index; i++) {
		if (ad->placelist[i] == NULL) {
			for (j = i; j < ad->last_index; j++) {
				ad->placelist[j] = ad->placelist[j+1];
			}
		backup_last--;
		}
	}
	ad->last_index = backup_last;

	elm_naviframe_item_pop(ad->nf);
}

void myplace_delete_cb(void *data, Evas_Object *obj, void *event_info)
{
	LS_FUNC_ENTER

	myplace_app_data *ad = (myplace_app_data *)data;

	Evas_Object *genlist;
	Elm_Genlist_Item_Class *itc_select_all;
	Evas_Object *done_btn, *cancel_btn;
	Evas_Object *check = NULL, *all_check = NULL;

	long int i;

	if (ad->ctx_popup) {
		evas_object_del(ad->ctx_popup);
		ad->ctx_popup = NULL;
	}

	/* genList */
	genlist = elm_genlist_add(ad->nf);
	elm_genlist_mode_set(genlist, ELM_LIST_COMPRESS);

	evas_object_data_set(genlist, "check", check);
	evas_object_data_set(genlist, "all_check", all_check);
	evas_object_data_set(genlist, "app_data", ad);

	itc_select_all = elm_genlist_item_class_new();
	if (itc_select_all == NULL)
		return;
	itc_select_all->item_style = "type1";
	itc_select_all->func.text_get = delete_name_text_get;
	itc_select_all->func.content_get = delete_all_check_get;
	itc_select_all->func.state_get = NULL;
	itc_select_all->func.del = NULL;
	ad->gi_del_all = elm_genlist_item_append(genlist, itc_select_all, (void *)-1, NULL, ELM_GENLIST_ITEM_NONE, set_checkall_by_genlist_cb, NULL);

	for (i = DEFAULT_PLACE_COUNT; i <= ad->last_index; i++) {
		ad->placelist[i]->del_check = EINA_FALSE;

		ad->placelist[i]->itc_delplace = elm_genlist_item_class_new();
		if (ad->placelist[i]->itc_delplace == NULL)
			return;
		ad->placelist[i]->itc_delplace->item_style = "type1";
		ad->placelist[i]->itc_delplace->func.text_get = myplace_place_text_get;
		ad->placelist[i]->itc_delplace->func.content_get = delete_check_get;
		ad->placelist[i]->itc_delplace->func.state_get = NULL;
		ad->placelist[i]->itc_delplace->func.del = NULL;
		ad->placelist[i]->gi_delplace = elm_genlist_item_append(genlist, ad->placelist[i]->itc_delplace, (void *)i, NULL, ELM_GENLIST_ITEM_NONE, set_check_by_genlist_cb, (void *)i);
	}

	evas_object_show(genlist);

	ad->nf_it = elm_naviframe_item_push(ad->nf, NULL, NULL, NULL, genlist, NULL);
	__update_title(ad);

	/* title cancel button */
	cancel_btn = elm_button_add(ad->nf);
	elm_object_style_set(cancel_btn, "naviframe/title_left");
	elm_object_part_text_set(cancel_btn, "default", P_("IDS_COM_SK_CANCEL"));
	evas_object_smart_callback_add(cancel_btn, "clicked", delete_cancel_cb, ad);
	elm_object_item_part_content_set(ad->nf_it, "title_left_btn", cancel_btn);

	/* title done button */
	done_btn = elm_button_add(ad->nf);
	elm_object_style_set(done_btn, "naviframe/title_right");
	elm_object_part_text_set(done_btn, "default", P_("IDS_COM_BODY_DONE"));
	evas_object_smart_callback_add(done_btn, "clicked", delete_done_cb, ad);
	elm_object_item_part_content_set(ad->nf_it, "title_right_btn", done_btn);
}
