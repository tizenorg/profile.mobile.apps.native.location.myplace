/*
 *  lbs-setting
 *
 * Copyright (c) 2000 - 2011 Samsung Electronics Co., Ltd. All rights reserved.
 *
 * Contact: Kyoungjun Sung <kj7.sung@samsung.com>, Young-Ae Kang <youngae.kang@samsung.com>
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

#include <Elementary.h>
#include <locations.h>
#include <vconf.h>
#include "location-test.h"
#include "myplace-common.h"


void view_log(Evas_Object *view, const char *fmt, ...);


static Evas_Object *vlog = NULL, *type_pop = NULL;
static int ret, i;
static location_manager_h manager = NULL;
static myplace_app_data *ad = NULL;

static char *TYPE_METHOD[] ={"HYBRID", "GPS", "WPS", "MOCK"};
static char *TYPE_INTERVAL[] ={"1", "2", "3", "4", "5", "10", "20"};
static char *TYPE_PERIOD[] ={"1", "2", "3", "5", "7", "10", "20", "50", "120"};
static int INT_INTERVAL[]	= { 1, 2, 3, 4, 5, 10, 20};
static int INT_PERIOD[]	= { 1, 2, 3, 5, 7, 10, 20, 50, 120};
static int METHOD = 0, INTERVAL = 2, PERIOD = 3;
static int num_of_active = 0, num_of_inview = 0;
static int create_flag = FALSE, start_flag = FALSE, event_flag = 0;
static double lat = 0.0, alt = 0.0, lon = 0.0;
static double cli = 0.0, dir = 0.0, spd = 0.0, hor = 0.0, ver = 0.0;
static time_t timestamp = 0.0;
static location_accuracy_level_e level;
static bool cb_pos = false, cb_vel = false, cb_loc = false, cb_dist = false, cb_sat = false;

static void _move_ctxpopup(Evas_Object *ctx, Evas_Object *btn)
{
	Evas_Coord x, y, w , h;
	evas_object_geometry_get(btn, &x, &y, &w, &h);
	evas_object_move(ctx, 100, 100);
}

static void _popup_method_cb(void *data, Evas_Object *obj, void *event_info)
{
	METHOD = (int) data;
	evas_object_del(type_pop);
	view_log(vlog, "<font_size=17>Method: [%s]<br></font_size>", TYPE_METHOD[METHOD]);
}
static void _popup_interval_cb(void *data, Evas_Object *obj, void *event_info)
{
	INTERVAL = INT_INTERVAL[(int)data];
	evas_object_del(type_pop);
	view_log(vlog, "<font_size=17>Interval: %d<br></font_size>", INTERVAL);
}
static void _popup_period_cb(void *data, Evas_Object *obj, void *event_info)
{
	PERIOD = INT_PERIOD[(int)data];
	evas_object_del(type_pop);
	view_log(vlog, "<font_size=17>Interval: %d, Period: %d<br></font_size>", INTERVAL, PERIOD);
}


//--------------------------------------------------------------------------
static void _popup_method(void *data, Evas_Object *obj, void *event_info)
{
	if (type_pop) {
		evas_object_del(type_pop); type_pop = NULL;
	}
	type_pop = elm_ctxpopup_add(ad->nf);

	for(i = 0; i < sizeof(TYPE_METHOD) / sizeof(TYPE_METHOD[0]); i++)
		elm_ctxpopup_item_append(type_pop, TYPE_METHOD[i], NULL, _popup_method_cb, (void *)i);

	_move_ctxpopup(type_pop, obj);
	evas_object_show(type_pop);
}
static void _popup_interval(void *data, Evas_Object *obj, void *event_info)
{
	if (type_pop) {
		evas_object_del(type_pop); type_pop = NULL;
	}
	type_pop = elm_ctxpopup_add(ad->nf);

	for(i = 0; i < sizeof(TYPE_INTERVAL) / sizeof(TYPE_INTERVAL[0]); i++)
		elm_ctxpopup_item_append(type_pop, TYPE_INTERVAL[i], NULL, _popup_interval_cb, (void *)i);

	_move_ctxpopup(type_pop, obj);
	evas_object_show(type_pop);
}
static void _popup_period(void *data, Evas_Object *obj, void *event_info)
{
	if (type_pop) {
		evas_object_del(type_pop); type_pop = NULL;
	}
	type_pop = elm_ctxpopup_add(ad->nf);

	for(i = 0; i < sizeof(TYPE_PERIOD) / sizeof(TYPE_PERIOD[0]); i++)
		elm_ctxpopup_item_append(type_pop, TYPE_PERIOD[i], NULL, _popup_period_cb, (void *)i);

	_move_ctxpopup(type_pop, obj);
	evas_object_show(type_pop);
}

//--------------------------------------------------------------------------
static void _popup_start_stop_cb(void *data, Evas_Object *obj, void *event_info)
{
	int idx = (int) data;
	evas_object_del(type_pop);

	if (idx == 0) {
		ret = location_manager_create(METHOD, &manager);
		view_log(vlog, "<font_size=17>Location_create.. </font_size>");
	} else if (idx == 1) {
		ret = location_manager_set_service_state_changed_cb(manager, __state_changed_cb, NULL);
		view_log(vlog, "<font_size=15>set_state_changed_cb.. </font_size>");
	} else if (idx == 2) {
		ret = location_manager_start(manager);
		view_log(vlog, "<font_size=17>Location_start.. </font_size>");
	} else if (idx == 3) {
		ret = location_manager_stop(manager);
		view_log(vlog, "<font_size=17>Location_stop.. </font_size>");
	} else if (idx == 4) {
		ret = location_manager_destroy(manager);
		view_log(vlog, "<font_size=17>Location_destroy.. </font_size>");
	} else if (idx == 5) {
//		ret = location_manager_request_single_location(manager, 90, cb, NULL);
		view_log(vlog, "<font_size=17>Single request is not ready.. </font_size>");
	}
	show_result(ret, ad);
}
static void _popup_start_stop(void *data, Evas_Object *obj, void *event_info)
{
	if (type_pop) {
		evas_object_del(type_pop); type_pop = NULL;
	}
	type_pop = elm_ctxpopup_add(ad->nf);

	char *TYPE_API[] ={"create", "state_changed_cb", "start", "stop", "destroy", "Single request"};
	for(i = 0; i < sizeof(TYPE_API) / sizeof(TYPE_API[0]); i++)
		elm_ctxpopup_item_append(type_pop, TYPE_API[i], NULL, _popup_start_stop_cb, (void *)i);

	_move_ctxpopup(type_pop, obj);
	evas_object_show(type_pop);
}

//--------------------------------------------------------------------------
static void _popup_callback_cb(void *data, Evas_Object *obj, void *event_info)
{
	int idx = (int) data;
	evas_object_del(type_pop);

	if (idx == 0) {
		view_log(vlog, "<font_size=17>position_cb.. </font_size>");
		if (cb_pos) {	ret = location_manager_unset_position_updated_cb(manager); cb_pos = false;
		} else {		ret = location_manager_set_position_updated_cb(manager, __position_updated_cb, INTERVAL, NULL); cb_pos = true;
		}

	} else if (idx == 1) {
		view_log(vlog, "<font_size=17>velocity_cb.. </font_size>");
		if (cb_vel) {	ret = location_manager_unset_velocity_updated_cb(manager); cb_vel = false;
		} else {		ret = location_manager_set_velocity_updated_cb(manager, __velocity_updated_cb, INTERVAL, NULL); cb_vel = true;
		}

	} else if (idx == 2) {
		view_log(vlog, "<font_size=15>location_changed_cb.. </font_size>");
		if (cb_loc) {	ret = location_manager_unset_location_changed_cb(manager); cb_loc = false;
		} else {		ret = location_manager_set_location_changed_cb(manager, __location_changed_cb, INTERVAL, NULL); cb_loc = true;
		}

	} else if (idx == 3) {
		view_log(vlog, "<font_size=17>distance_cb.. </font_size>");
		if (cb_dist) {	ret = location_manager_unset_distance_based_location_changed_cb(manager); cb_dist = false;
		} else {		ret = location_manager_set_distance_based_location_changed_cb(manager, __distance_changed_cb, INTERVAL, 20, NULL); cb_dist = true;
		}

	} else if (idx == 4) {
		view_log(vlog, "<font_size=17>satellite_cb.. </font_size>");
		if (cb_sat) {	ret = gps_status_unset_satellite_updated_cb(manager); cb_sat = false;
		} else {		ret = gps_status_set_satellite_updated_cb(manager, __satellite_updated_cb, INTERVAL, NULL); cb_sat = true;
		}
	}
	show_result(ret, ad);
}
static void _popup_callback(void *data, Evas_Object *obj, void *event_info)
{
	if (type_pop) {
		evas_object_del(type_pop); type_pop = NULL;
	}
	type_pop = elm_ctxpopup_add(ad->nf);

	char *TYPE_API[] ={"position_cb", "velocity_cb", "location_cb", "distance_cb", "satellite_cb"};
	for(i = 0; i < sizeof(TYPE_API) / sizeof(TYPE_API[0]); i++)
		elm_ctxpopup_item_append(type_pop, TYPE_API[i], NULL, _popup_callback_cb, (void *)i);

	_move_ctxpopup(type_pop, obj);
	evas_object_show(type_pop);
}

//--------------------------------------------------------------------------
static void _popup_get_cb(void *data, Evas_Object *obj, void *event_info)
{
	int idx = (int) data;
	evas_object_del(type_pop);

	if (idx == 0) {
		ret = location_manager_get_position(manager, &alt, &lat, &lon, &timestamp);
		view_log(vlog, "<font_size=17>get_pos [lat: %.1lf, lon: %.1lf, alt: %.1lf, time: %d].. </font_size>", lat, lon, alt, timestamp);
	} else if (idx == 1) {
		ret = location_manager_get_location(manager, &alt, &lat, &lon, &cli, &dir, &spd, &level, &hor, &ver, &timestamp);
		view_log(vlog, "<font_size=17>get_loc [lat: %.1lf, lon: %.1lf, alt: %.1lf, time: %d].. </font_size>", lat, lon, alt, timestamp);
	} else if (idx == 2) {
		ret = location_manager_get_velocity(manager, &cli, &dir, &spd, &timestamp);
		view_log(vlog, "<font_size=15>get_vel [cli: %.1lf, dir: %.1lf, speed: %.1lf] .. </font_size>", cli, dir, spd);
	} else if (idx == 3) {
		ret = location_manager_get_accuracy(manager, &level, &hor, &ver);
		view_log(vlog, "<font_size=17>get_acc [hor: %.1lf, ver: %.1lf].. </font_size>", hor, ver);
	} else if (idx == 4) {
		ret = location_manager_get_last_position(manager, &alt, &lat, &lon, &timestamp);
		view_log(vlog, "<font_size=17>last_pos [lat: %.1lf, lon: %.1lf, alt: %.1lf, time: %d].. </font_size>", lat, lon, alt, timestamp);
	} else if (idx == 5) {
		ret = location_manager_get_last_location(manager, &alt, &lat, &lon, &cli, &dir, &spd, &level, &hor, &ver, &timestamp);
		view_log(vlog, "<font_size=17>last_loc [lat: %.1lf, lon: %.1lf, alt: %.1lf, time: %d].. </font_size>", lat, lon, alt, timestamp);
	} else if (idx == 6) {
		ret = location_manager_get_last_velocity(manager, &cli, &dir, &spd, &timestamp);
		view_log(vlog, "<font_size=15>last_vel [cli: %.1lf, dir: %.1lf, speed: %.1lf] .. </font_size>", cli, dir, spd);
	} else if (idx == 7) {
		ret = location_manager_get_last_accuracy(manager, &level, &hor, &ver);
		view_log(vlog, "<font_size=17>last_acc [hor: %.1lf, ver: %.1lf].. </font_size>", hor, ver);
	}
	show_result(ret, ad);
}
static void _popup_get(void *data, Evas_Object *obj, void *event_info)
{
	if (type_pop) {
		evas_object_del(type_pop); type_pop = NULL;
	}
	type_pop = elm_ctxpopup_add(ad->nf);

	char *TYPE_API[] ={"position", "location", "velocity", "accuracy", "last_pos", "last_loc", "last_vel", "last_acc"};
	for(i = 0; i < sizeof(TYPE_API) / sizeof(TYPE_API[0]); i++)
		elm_ctxpopup_item_append(type_pop, TYPE_API[i], NULL, _popup_get_cb, (void *)i);

	_move_ctxpopup(type_pop, obj);
	evas_object_show(type_pop);
}

//--------------------------------------------------------------------------
static void _popup_batch_cb(void *data, Evas_Object *obj, void *event_info)
{
	int idx = (int) data;
	evas_object_del(type_pop);

	if (idx == 0) {
		view_log(vlog, "<font_size=17>Location_create[GPS].. </font_size>");
		ret = location_manager_create(LOCATIONS_METHOD_GPS, &manager);
	} else if (idx == 1) {
		view_log(vlog, "<font_size=17>State_changed_cb ... </font_size>");
		ret = location_manager_set_service_state_changed_cb(manager, __state_changed_cb, NULL);
	} else if (idx == 2) {
		view_log(vlog, "<font_size=17>Location_batch_cb ... </font_size>");
		ret = location_manager_set_location_batch_cb(manager, __batch_updated_cb, INTERVAL, PERIOD, NULL);
	} else if (idx == 3) {
		view_log(vlog, "<font_size=17>Location_start_batch ... </font_size>");
		ret = location_manager_start_batch(manager);
	} else if (idx == 4) {
		view_log(vlog, "<font_size=17>Batch_foreach.. </font_size>");
		ret = location_manager_foreach_location_batch(manager, __get_batch_cb, NULL);
	} else if (idx == 5) {
		view_log(vlog, "<font_size=17>Batch_stop.. </font_size>");
		ret = location_manager_stop_batch(manager);
	} else if (idx == 6) {
		view_log(vlog, "<font_size=17>Batch_unset & destroy .. </font_size>");
		location_manager_unset_location_batch_cb(manager);
		ret = location_manager_destroy(manager);
	}
	show_result(ret, ad);
}
static void _popup_batch(void *data, Evas_Object *obj, void *event_info)
{
	if (type_pop) {
		evas_object_del(type_pop); type_pop = NULL;
	}
	type_pop = elm_ctxpopup_add(ad->nf);

	char *TYPE_API[] ={"create_Gps", "service_cb", "batch_cb", "start_batch", "foreach", "stop_batch", "destroy"};
	for(i = 0; i < sizeof(TYPE_API) / sizeof(TYPE_API[0]); i++)
		elm_ctxpopup_item_append(type_pop, TYPE_API[i], NULL, _popup_batch_cb, (void *)i);

	_move_ctxpopup(type_pop, obj);
	evas_object_show(type_pop);
}

//--------------------------------------------------------------------------
static void _popup_satellite_cb(void *data, Evas_Object *obj, void *event_info)
{
	int idx = (int) data;
	evas_object_del(type_pop);

	if (idx == 0) {
	    ret = gps_status_get_satellite(manager, &num_of_active, &num_of_inview, &timestamp);
	    view_log(vlog, "<font_size=15>get_satellite ... [active:%d, inview:%d] ... </font_size>", num_of_active, num_of_inview);
	} else if (idx == 1) {
		view_log(vlog, "<font_size=17>Satellite_foreach ... </font_size>");
		ret = gps_status_foreach_satellites_in_view(manager, __get_satellites_cb, ad);
	}
	show_result(ret, ad);
}
static void _popup_satellite(void *data, Evas_Object *obj, void *event_info)
{
	if (type_pop) {
		evas_object_del(type_pop); type_pop = NULL;
	}
	type_pop = elm_ctxpopup_add(ad->nf);

	char *TYPE_API[] ={"get_satellite", "foreach"};
	for(i = 0; i < sizeof(TYPE_API) / sizeof(TYPE_API[0]); i++)
		elm_ctxpopup_item_append(type_pop, TYPE_API[i], NULL, _popup_satellite_cb, (void *)i);

	_move_ctxpopup(type_pop, obj);
	evas_object_show(type_pop);
}

//--------------------------------------------------------------------------
static const char* get_state(int state)
{
	switch (state) {
	case 0: return "OFF";
	case 1: return "SEARCHING";
	case 2: return "CONNECTED";
	default: return "INVALID";
	}
}

static void _show_position_status()
{
	int pos, gps, wps;
	vconf_get_int(VCONFKEY_LOCATION_POSITION_STATE, &pos);
	vconf_get_int(VCONFKEY_LOCATION_GPS_STATE, &gps);
	vconf_get_int(VCONFKEY_LOCATION_WPS_STATE, &wps);
	view_log(vlog, "<font_size=20> =========== GPS [%s]   WPS [%s]   Pos [%s] </font_size>", get_state(gps), get_state(wps), get_state(pos));
	show_result(ret, ad);
}

static void _popup_etc(void *data, Evas_Object *obj, void *event_info){ _show_position_status(); }
static void _vconf_pos_changed_cb(keynode_t *key, void *data){ _show_position_status();}
static void _vconf_gps_changed_cb(keynode_t *key, void *data){ _show_position_status();}
static void _vconf_wps_changed_cb(keynode_t *key, void *data){ _show_position_status();}


static void __setting_location_back_cb(void *data, Evas_Object *obj, void *event_info)
{
	LS_FUNC_ENTER
	LS_RETURN_IF_FAILED(data);

	vconf_ignore_key_changed(VCONFKEY_LOCATION_POSITION_STATE, _vconf_pos_changed_cb);
	vconf_ignore_key_changed(VCONFKEY_LOCATION_GPS_STATE, _vconf_gps_changed_cb);
	vconf_ignore_key_changed(VCONFKEY_LOCATION_WPS_STATE, _vconf_wps_changed_cb);
	elm_naviframe_item_pop(ad->nf);
}

//void _setting_location_test_view(void *data)
void _setting_location_test_view(void *data, Evas_Object *obj, void *event_info)
{
	LS_FUNC_ENTER
	LS_RETURN_IF_FAILED(obj);
	ad = evas_object_data_get(obj, "app_data");
	Evas_Object *box, *method, *interval, *startstop, *callback, *get, *batch, *period, *satellite, *etc;

	box = elm_box_add(ad->nf);
	evas_object_size_hint_weight_set(box, EVAS_HINT_EXPAND, EVAS_HINT_EXPAND);
	evas_object_size_hint_align_set(box, EVAS_HINT_FILL, EVAS_HINT_FILL);

	method = elm_button_add(box);
	interval = elm_button_add(box);
	startstop = elm_button_add(box);
	callback = elm_button_add(box);
	get = elm_button_add(box);
	batch = elm_button_add(box);
	period = elm_button_add(box);
	satellite = elm_button_add(box);
	etc = elm_button_add(box);

///////////////////////////////////////////
	elm_object_text_set(method, ("<font_size=20>Method</font_size>"));
	evas_object_smart_callback_add(method, "clicked", _popup_method, ad);
	evas_object_resize(method, 120, 50); evas_object_move(method, 20, 150); evas_object_show(method);

	elm_object_text_set(interval, ("<font_size=20>Interval</font_size>"));
	evas_object_smart_callback_add(interval, "clicked", _popup_interval, ad);
	evas_object_resize(interval, 120, 50); evas_object_move(interval, 160, 150); evas_object_show(interval);

	elm_object_text_set(startstop, ("<font_size=20>StartStop</font_size>"));
	evas_object_smart_callback_add(startstop, "clicked", _popup_start_stop, ad);
	evas_object_resize(startstop, 120, 50); evas_object_move(startstop, 300, 150); evas_object_show(startstop);

	elm_object_text_set(callback, ("<font_size=20>Callback</font_size>"));
	evas_object_smart_callback_add(callback, "clicked", _popup_callback, ad);
	evas_object_resize(callback, 120, 50); evas_object_move(callback, 440, 150); evas_object_show(callback);

	elm_object_text_set(get, ("<font_size=20>Get</font_size>"));
	evas_object_smart_callback_add(get, "clicked", _popup_get, ad);
	evas_object_resize(get, 120, 50); evas_object_move(get, 580, 150); evas_object_show(get);

	elm_object_text_set(batch, ("<font_size=20>Batch</font_size>"));
	evas_object_smart_callback_add(batch, "clicked", _popup_batch, ad);
	evas_object_resize(batch, 120, 50); evas_object_move(batch, 20, 230); evas_object_show(batch);

	elm_object_text_set(period, ("<font_size=20>Period</font_size>"));
	evas_object_smart_callback_add(period, "clicked", _popup_period, ad);
	evas_object_resize(period, 120, 50); evas_object_move(period, 160, 230); evas_object_show(period);

	elm_object_text_set(satellite, ("<font_size=20>Satellite</font_size>"));
	evas_object_smart_callback_add(satellite, "clicked", _popup_satellite, ad);
	evas_object_resize(satellite, 120, 50); evas_object_move(satellite, 300, 230); evas_object_show(satellite);

	elm_object_text_set(etc, ("<font_size=20>Etc</font_size>"));
	evas_object_smart_callback_add(etc, "clicked", _popup_etc, ad);
	evas_object_resize(etc, 120, 50); evas_object_move(etc, 440, 230); evas_object_show(etc);

//	elm_object_text_set(, ("<font_size=20>StartStop</font_size>"));
//	evas_object_smart_callback_add(, "clicked", _popup_start_stop, ad);
//	evas_object_resize(, 120, 50); evas_object_move(, 300, 230); evas_object_show();

	vlog = elm_entry_add(box);
	elm_entry_scrollable_set(vlog, EINA_TRUE);
	elm_entry_editable_set(vlog, EINA_FALSE);
	elm_entry_input_panel_enabled_set(vlog, EINA_FALSE);
	evas_object_resize(vlog, 670, 920);
	evas_object_move(vlog, 30, 300);
	evas_object_show(vlog);

	vconf_notify_key_changed(VCONFKEY_LOCATION_POSITION_STATE, _vconf_pos_changed_cb, NULL);
	vconf_notify_key_changed(VCONFKEY_LOCATION_GPS_STATE, _vconf_gps_changed_cb, NULL);
	vconf_notify_key_changed(VCONFKEY_LOCATION_WPS_STATE, _vconf_wps_changed_cb, NULL);

	Evas_Object *back_btn = elm_button_add(ad->nf);
	elm_object_style_set(back_btn, "naviframe/back_btn/default");
	evas_object_smart_callback_add(back_btn, "clicked", __setting_location_back_cb, ad);

	evas_object_show(box);
	elm_naviframe_item_push(ad->nf, P_("Location Test"), back_btn, NULL, box, NULL);
}







/////////////////////////////  Update Callback  ///////////////////////////////
void __position_updated_cb(double latitude, double longitude, double altitude, time_t timestamp, void *user_data)
{
	int gps; vconf_get_int(VCONFKEY_LOCATION_GPS_STATE, &gps);	int wps; vconf_get_int(VCONFKEY_LOCATION_WPS_STATE, &wps);
	view_log(vlog, "<color=#3090C7><font_size=16>Pos [lat:%.1lf, log:%.1lf, alt:%.1lf, GPS[%d,%d] %d]<br></font_size></color>", latitude, longitude, altitude, gps, wps, timestamp);
}

void __velocity_updated_cb(double speed, double direction, double climb, time_t timestamp, void *user_data)
{
	int gps; vconf_get_int(VCONFKEY_LOCATION_GPS_STATE, &gps);
	view_log(vlog, "<color=#9090C7><font_size=16>VEL [spd:%.1lf, dir:%.1lf, climb:%.1lf, GPS[%d] %d]-------<br></font_size></color>", speed, direction, climb, gps, timestamp);
}

void __location_updated_cb(location_error_e error, double latitude, double longitude, double altitude, time_t timestamp, double speed, double direction, double climb, void *user_data)
{
	int gps; vconf_get_int(VCONFKEY_LOCATION_GPS_STATE, &gps);	int wps; vconf_get_int(VCONFKEY_LOCATION_WPS_STATE, &wps);
	view_log(vlog, "<color=#303077><font_size=16>LOCATION!! [Pos:%.1lf, %.1lf, %.1lf, Vel:%.1lf, %.1lf, %.1lf, GPS[%d, %d] %d]<br></font_size></color>", latitude, longitude, altitude, speed, direction, climb, gps, wps, timestamp);
}

void __location_changed_cb(double latitude, double longitude, double altitude, double speed, double direction, double hor_accuracy, time_t timestamp, void *user_data)
{
	int gps; vconf_get_int(VCONFKEY_LOCATION_GPS_STATE, &gps);	int wps; vconf_get_int(VCONFKEY_LOCATION_WPS_STATE, &wps);
	view_log(vlog, "<color=#406077><font_size=16>=== Changed!! [Pos:%.1lf, %.1lf, %.1lf, Vel:%.1lf, %.1lf, H_Acc%.1lf, GPS[%d, %d] %d]<br></font_size></color>", latitude, longitude, altitude, speed, direction, hor_accuracy, gps, wps, timestamp);
}

void __distance_changed_cb(double latitude, double longitude, double altitude, double speed, double direction, double hor_accuracy, time_t timestamp, void *user_data)
{
	view_log(vlog, "<color=#109017><font_size=16>Dist [Pos:%.1lf, %.1lf, %.1lf, Vel:%.1lf, %.1lf, H_Acc%.1lf]<br></font_size></color>", latitude, longitude, altitude, speed, direction, hor_accuracy);
}

void __location_AA_updated_cb(double latitude, double longitude, double altitude, time_t timestamp, void *user_data)
{
	int gps; vconf_get_int(VCONFKEY_LOCATION_GPS_STATE, &gps);
	view_log(vlog, "<color=#6060C7><font_size=16>AA_Pos [lat:%.1lf, lon:%.1lf, alt:%.1lf, GPS[%d] %d]<br></font_size></color>", latitude, longitude, altitude, gps, timestamp);
}


/////////////////////////////  G P S  ///////////////////////////////
void __satellite_updated_cb(int num_of_active, int num_of_inview, time_t timestamp, void *user_data)
{
	int gps; vconf_get_int(VCONFKEY_LOCATION_GPS_STATE, &gps);	int wps; vconf_get_int(VCONFKEY_LOCATION_WPS_STATE, &wps);
	view_log(vlog, "<color=#9F000F><font_size=16>Satellite_updated!! [active: %d, inview: %d, GPS[%d, %d] %d]<br></font_size></color>", num_of_active, num_of_inview, gps, wps, timestamp);
	ad->satellite_enable = true;
}

bool __get_satellites_cb(unsigned int azimuth, unsigned int elevation, unsigned int prn, int snr, bool is_in_use, void *user_data)
{
	int gps; vconf_get_int(VCONFKEY_LOCATION_GPS_STATE, &gps);
	view_log(vlog, "<color=#3090C7><font_size=16>get_satellite!! [azimuth:%d, elevation:%d, prn:%d, snr:%d, is_in_use:%d, GPS[%d]]<br></font_size></color>", azimuth, elevation, prn, snr, is_in_use, gps);
	return true;
}


/////////////////////////////  State & Setting  ///////////////////////////////
void __state_changed_cb(location_service_state_e state, void *user_data)
{
//	view_log(vlog, "<color=#9F000F><font_size=16>Service state changed!!<br></font_size></color>");

	if (state ==LOCATIONS_SERVICE_ENABLED) {
		view_log(vlog, "<color=#9F000F><font_size=16>Service state changed!! [%s]<br></font_size></color>", "SERVICE_ENABLED");
	} else {
		view_log(vlog, "<color=#9F000F><font_size=16>Service state changed!! [%s]<br></font_size></color>", "SERVICE_DISABLED");
	}
}

void __setting_changed_cb(location_method_e method, bool enable, void *user_data)
{
	const char * status = NULL;
	if (enable) status = "True";
	else status = "False";

	view_log(vlog, "<color=#9F000F><font_size=16>setting Changed!! [%s] %s<br></font_size></color>", TYPE_METHOD[method], status);
}


/////////////////////////////  Batch & Zone  ///////////////////////////////
void __zone_changed_cb(location_boundary_state_e state, double latitude, double longitude, double altitude, time_t timestamp, void *user_data)
{
	view_log(vlog, "<color=#9F000F><font_size=16>ZoneChanged!! [State: %d, Lat:%lf, Lon:%lf, Alt:%lf]<br></font_size></color>", state, latitude, longitude, altitude);
}

void __batch_updated_cb(int num_of_location, void *user_data)
{
	view_log(vlog, "<color=#9F000F><font_size=16>Batch_updated!! [num_of_batch data: %d]<br></font_size></color>", num_of_location);
}

bool __get_batch_cb(double latitude, double longitude, double altitude, double speed, double direction, double horizontal, double vertical, time_t timestamp, void *user_data)
{
	view_log(vlog, "<color=#9F000F><font_size=16>Get batch [%ld, %lf, %lf, %lf, %lf, %lf, %lf, %lf]<br></font_size></color>", timestamp, latitude, longitude, altitude, speed, direction, horizontal, vertical);
	return true;
}


void show_result(int result, void *data)
{
	if (result == LOCATIONS_ERROR_NONE)						view_log(vlog, "<color=#3090C7><font_size=15>%s<br></font_size></color>", "ok");
	else if (result == LOCATIONS_ERROR_OUT_OF_MEMORY)		view_log(vlog, "<color=#9F000F><font_size=15>[%s]<br></font_size></color>", "OUT_OF_MEMORY");
	else if (result == LOCATIONS_ERROR_INVALID_PARAMETER)	view_log(vlog, "<color=#9F000F><font_size=15>[%s]<br></font_size></color>", "INVALID_PARAMETER");
	else if (result == LOCATIONS_ERROR_INCORRECT_METHOD)	view_log(vlog, "<color=#9F000F><font_size=15>[%s]<br></font_size></color>", "INCORRECT_METHOD");
	else if (result == LOCATIONS_ERROR_NETWORK_FAILED)		view_log(vlog, "<color=#9F000F><font_size=15>[%s]<br></font_size></color>", "NETWORK_FAILED");
	else if (result == LOCATIONS_ERROR_SERVICE_NOT_AVAILABLE)	view_log(vlog, "<color=#9F000F><font_size=15>[%s]<br></font_size></color>", "SERVICE_NOT_AVAILABLE");
	else if (result == LOCATIONS_ERROR_GPS_SETTING_OFF)		view_log(vlog, "<color=#9F000F><font_size=15>[%s]<br></font_size></color>", "GPS_SETTING_OFF");
	else if (result == LOCATIONS_ERROR_SECURITY_RESTRICTED)	view_log(vlog, "<color=#9F000F><font_size=15>[%s]<br></font_size></color>", "SECURITY_RESTRICTED");
	else if (result == LOCATIONS_ERROR_ACCESSIBILITY_NOT_ALLOWED) view_log(vlog, "<color=#9F000F><font_size=15>[%s]<br></font_size></color>", "ACCESSIBILITY_NOT_ALLOWED");
	else if (result == LOCATIONS_ERROR_NOT_SUPPORTED) view_log(vlog, "<color=#9F000F><font_size=15>[%s]<br></font_size></color>", "ERROR_NOT_SUPPORTED");
	else if (result == LOCATIONS_ERROR_SETTING_OFF) view_log(vlog, "<color=#9F000F><font_size=15>[%s]<br></font_size></color>", "ERROR_SETTING_OFF");
}


void view_log(Evas_Object *view, const char *fmt, ...)
{
	va_list arg;
	char buffer[2024] = {0, };
	va_start(arg, fmt);
	vsnprintf(buffer,2024,fmt,arg);
	elm_entry_cursor_end_set(view);
	elm_entry_entry_insert(view, buffer);
	va_end(arg);
}

