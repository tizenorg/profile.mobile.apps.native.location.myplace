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
#include <vconf.h>
#include <Elementary.h>
#include <Elementary.h>
#include <libintl.h>
#include <locations.h>
#include <dlog.h>

#include "myplace-common.h"
#include "myplace-placelist.h"

static myplace_app_data *global_ad;

myplace_app_data *myplace_common_get_app_data(void)
{
	LS_FUNC_ENTER
	return global_ad;
}

void myplace_common_destroy_app_data(void)
{
	LS_FUNC_ENTER

	return;
}

void _myplace_win_quit_cb(void *data, Evas_Object *obj, void *event_info)
{
	LS_FUNC_ENTER
	myplace_app_data *appData = (myplace_app_data *)data;
	LS_RETURN_IF_FAILED(appData);
	LS_LOGD("delete,request");
	appData->win_main = NULL;

	myplace_common_destroy_app_data();
	elm_exit();
}

static bool _app_create_cb(void *user_data)
{
	LS_FUNC_ENTER

	return true;
}

static void _app_terminate_cb(void *user_data)
{
	LS_FUNC_ENTER
	myplace_app_data *ad = (myplace_app_data *) user_data;

	if (ad->maps_view != NULL) {
		maps_view_remove_all_objects(ad->maps_view);
		maps_view_destroy(ad->maps_view);
		ad->maps_view = NULL;
	}
	if (ad->maps_service != NULL) {
		maps_service_destroy(ad->maps_service);
		ad->maps_service = NULL;
	}
	if (ad->geo_manager != NULL)
		geofence_manager_destroy(ad->geo_manager);

	ad->geo_manager = NULL;
}

/*
static void _app_pause_cb(void *user_data)
{
	LS_FUNC_ENTER
}

static void _app_resume_cb(void *user_data)
{
	LS_FUNC_ENTER
}
*/

static void _app_control_cb(app_control_h app_control, void *user_data)
{
	LS_FUNC_ENTER

	gboolean ret = FALSE;
	myplace_app_data *ad = (myplace_app_data *) user_data;
	LS_RETURN_IF_FAILED(ad);

	if (ad->win_main) {
		evas_object_del(ad->win_main);
		ad->win_main = NULL;
	}

	elm_config_accel_preference_set("opengl");
	ad->geo_manager = NULL;

	app_control_get_extra_data(app_control, MYPLACE_CALLER, &(ad->caller));
	LS_LOGE("%s", ad->caller);

	bindtextdomain(MYPLACE_PKG, LOCALE_DIR);

	ad->win_main = create_win(MYPLACE_PKG);
	ad->bg = create_bg(ad->win_main);
	ad->conformant = create_conformant(ad->win_main);

	create_indicator_bg(ad->conformant);
	ad->layout_main = create_layout(ad->conformant);
	ret = app_control_clone(&ad->prev_handler, app_control);
	if (ret == FALSE)
		LS_LOGE("app_control_clone. err=%d", ret);

	myplace_geofence_init(ad);
	myplace_placelist_cb(ad);

	LS_FUNC_EXIT
}

/*
static void _app_low_memory_cb(void *user_data)
{
	LS_FUNC_ENTER
}

static void _app_low_battery_cb(void *user_data)
{
	LS_FUNC_ENTER
}

static void _app_device_orientation_cb(app_event_info_h event_info, void *user_data)
{
	LS_FUNC_ENTER
	LS_RETURN_IF_FAILED(event_info);
	LS_RETURN_IF_FAILED(user_data);

	myplace_app_data *ad = (myplace_app_data *)user_data;
	app_device_orientation_e orientation;
	Evas_Object *panel = NULL;

	app_event_get_device_orientation(event_info, &orientation);
}
*/

static void _app_language_changed_cb(app_event_info_h event_info, void *user_data)
{
	LS_FUNC_ENTER

	char *locale = vconf_get_str(VCONFKEY_LANGSET);
	if (locale) elm_language_set(locale);
}

int main(int argc, char *argv[])
{
	LS_FUNC_ENTER

	int ret = 0;
	myplace_app_data ad = {0,};

	ui_app_lifecycle_callback_s event_callback = {0,};
	app_event_handler_h handlers[5] = {NULL, };

	event_callback.create = _app_create_cb;
	event_callback.terminate = _app_terminate_cb;
	event_callback.app_control = _app_control_cb;

	ui_app_add_event_handler(&handlers[APP_EVENT_LOW_MEMORY], APP_EVENT_LOW_MEMORY, NULL, NULL);
	ui_app_add_event_handler(&handlers[APP_EVENT_LOW_BATTERY], APP_EVENT_LOW_BATTERY, NULL, NULL);
	ui_app_add_event_handler(&handlers[APP_EVENT_DEVICE_ORIENTATION_CHANGED], APP_EVENT_DEVICE_ORIENTATION_CHANGED, NULL, &ad);
	ui_app_add_event_handler(&handlers[APP_EVENT_LANGUAGE_CHANGED], APP_EVENT_LANGUAGE_CHANGED,	_app_language_changed_cb, NULL);
	ui_app_add_event_handler(&handlers[APP_EVENT_REGION_FORMAT_CHANGED], APP_EVENT_REGION_FORMAT_CHANGED, NULL, NULL);

	ret = APP_ERROR_NONE;
	ret = ui_app_main(argc, argv, &event_callback, &ad);

	if (ret != APP_ERROR_NONE)
		LS_LOGE("app_efl_main() is failed. err=%d", ret);

	return ret;

	LS_FUNC_EXIT
}
