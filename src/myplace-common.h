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



#ifndef MYPLACE_COMMON_H_
#define MYPLACE_COMMON_H_

#ifdef __cplusplus
extern "C" {
#endif

#include <app.h>
#include <dlog.h>
#include <glib.h>
#ifndef Eina_Bool
#include <stdbool.h>
#endif
#include <stdio.h>
#include <string.h>
#include <efl_extension.h>
#include <system_info.h>
#include <locations.h>

#include <wifi.h>
#include <bluetooth.h>
#include <geofence_manager.h>
#include <maps_view.h>

#if !defined(MYPLACE_PKG)
#define MYPLACE_PKG "org.tizen.myplace"
#endif

#define DOMAIN_NAME MYPLACE_PKG

#define TAG_MYPLACE "MYPLACE"
#define MYPLACE_CALLER "caller"
#define MYPLACE_DLOG_DEBUG

#ifdef MYPLACE_DLOG_DEBUG        /**< if debug mode, show filename & line number */

#ifdef LOG_TAG
#undef LOG_TAG
#define LOG_TAG TAG_MYPLACE
#endif

#define LS_LOGD(fmt,args...)  LOGD(fmt, ##args)
#define LS_LOGW(fmt,args...)  LOGW(fmt, ##args)
#define LS_LOGI(fmt,args...)  LOGI(fmt, ##args)
#define LS_LOGE(fmt,args...)  LOGE(fmt, ##args)

#elif MYPLACE_DLOG_RELEASE      /* if release mode */

#ifdef LOG_TAG
#undef LOG_TAG
#define LOG_TAG TAG_MYPLACE
#endif

#define LS_LOGD(fmt,args...)  LOGD(fmt, ##args)
#define LS_LOGW(fmt,args...)  LOGW(fmt, ##args)
#define LS_LOGI(fmt,args...)  LOGI(fmt, ##args)
#define LS_LOGE(fmt,args...)  LOGE(fmt, ##args)
#else                       /* if do not use dlog */
#define LS_LOGD(...)  g_debug(__VA_ARGS__)
#define LS_LOGW(...)  g_warning(__VA_ARGS__)
#define LS_LOGI(...)  g_message(__VA_ARGS__)
#define LS_LOGE(...)  g_error(__VA_ARGS__)
#endif

#define P_(s)			dgettext(MYPLACE_PKG, s)
#define S_(s)			dgettext("sys_string", s)
#define dgettext_noop(s)	(s)
#define N_(s)			dgettext_noop(s)

#define KEY_ENABLED	1
#define KEY_DISABLED 0

#define LS_FUNC_ENTER 	LS_LOGD("(%s) ENTER", __FUNCTION__);
#define LS_FUNC_EXIT 	LS_LOGD("(%s) EXIT", __FUNCTION__);

#define COLOR_TABLE "/usr/apps/org.tizen.myplace/res/myplace_ChangeableColorInfo.xml"
#define FONT_TABLE "/usr/apps/org.tizen.myplace/res/myplace_ChangeableFontInfo.xml"

#define SAFE_STRDUP(src) (src) ? strdup(src) : NULL

#define LS_MEM_FREE(ptr)	\
	do { \
		if (ptr != NULL) {	\
			free((void *)ptr);	\
			ptr = NULL;	\
		}	\
	} while (0)


#define LS_MEM_NEW(ptr, num_elements, type)	 \
	do { \
		if ((int)(num_elements) <= 0) { \
			ptr = NULL; \
		} else { \
			ptr = (type *) calloc(num_elements, sizeof(type)); \
		} \
	} while (0)


#define LS_RETURN_IF_FAILED(point) do { \
		if (point == NULL) { \
			LS_LOGE("critical error : LS_RETURN_IF_FAILED"); \
			return; \
		} \
	} while (0)

#define LS_RETURN_VAL_IF_FAILED(point, val) do { \
		if (point == NULL) { \
			LS_LOGE("critical error : NAVI_RETURN_VAL_IS_FAILED"); \
			return val; \
		} \
	} while (0)

#define DEFAULT_PLACE_COUNT 3
#define MAX_PLACE_COUNT 30
#define PLACE_GEOPOINT_RADIUS 200

#define PLACE_ID_HOME 1
#define PLACE_ID_OFFICE 2
#define PLACE_ID_CAR 3

typedef enum {
	MYPLACE_METHOD_NONE = 0,
	MYPLACE_METHOD_MAP,
	MYPLACE_METHOD_WIFI,
	MYPLACE_METHOD_BT,
	MYPLACE_METHOD_INVALID,
} myplace_method_index_e;

typedef struct myplace_data
{
	Eina_Bool del_check;
	Elm_Genlist_Item_Class *itc_myplace;
	Elm_Genlist_Item_Class *itc_delplace;
	Elm_Object_Item *gi_myplace;
	Elm_Object_Item *gi_delplace;

	int place_id;
	int map_fence_id, wifi_fence_id, bt_fence_id;
	char *name;

	bool is_default;

	double latitude, longitude;
	char *address;
	char *wifi_bssid, *wifi_ssid;
	char *bt_mac_address, *bt_ssid;

	bool method_map;
	bool method_wifi;
	bool method_bt;

	geofence_h map_geofence_params;
	geofence_h wifi_geofence_params;
	geofence_h bt_geofence_params;
} myplace_data;

typedef struct appdata
{
	app_control_h prev_handler;

	Evas_Object *win_main;
	Evas_Object *conformant;
	Evas_Object *bg;
	Evas_Object *layout_main;
	Evas_Object *nf;

	Evas_Object *place_genlist;
	Evas_Object *fence_genlist;
	Evas_Object *popup;
	Evas_Object *ctx_popup;
	Evas_Object *up_btn;
	Evas_Object *geofence_check;
	Evas_Object *map;
	Evas_Object *map_entry;
	Evas_Object *map_cancel_btn;
	Evas_Object *map_done_btn;

	Elm_Object_Item *gi_myplace;
	Elm_Object_Item *gi_map_method;
	Elm_Object_Item *gi_wifi_method;
	Elm_Object_Item *gi_bt_method;
	Elm_Object_Item *gi_del_all;

	Elm_Genlist_Item_Class *itc_myplace;

	Elm_Object_Item *nf_it;

	int is_geofence;
	long int last_index;
	int current_index;
	char *caller;

	geofence_manager_h geo_manager;

	myplace_data *placelist[MAX_PLACE_COUNT];
	myplace_data *selected_place;
	myplace_data *modified_place;
	myplace_data *mapview_place;

	maps_service_h maps_service;
	maps_view_h maps_view;
} myplace_app_data;

myplace_app_data *myplace_common_get_app_data(void);
void myplace_common_destroy_app_data(void);

#ifdef __cplusplus
}
#endif /*__cplusplus */

#endif /* MYPLACE_COMMON_H_ */
