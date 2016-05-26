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

#ifndef __LBS_SETTING_TEST_H__
#define __LBS_SETTING_TEST_H__

void _setting_location_test_view(void *data, Evas_Object *obj, void *event_info);


void error_msg(int error, int context);

void show_result(int result, void *data);
void __satellite_updated_cb(int num_of_active, int num_of_inview, time_t timestamp, void *user_data);
bool __get_satellites_cb(unsigned int azimuth, unsigned int elevation, unsigned int prn, int snr, bool is_in_use, void *user_data);
void __location_changed_cb(double latitude, double longitude, double altitude, double speed, double direction, double hor_accuracy, time_t timestamp, void *user_data);
void __distance_changed_cb(double latitude, double longitude, double altitude, double speed, double direction, double hor_accuracy, time_t timestamp, void *user_data);
void __location_updated_cb(location_error_e error, double latitude, double longitude, double altitude, time_t timestamp, double speed, double direction, double climb, void *user_data);
void __velocity_updated_cb(double speed, double direction, double climb, time_t timestamp, void *user_data);
void __location_AA_updated_cb(double latitude, double longitude, double altitude, time_t timestamp, void *user_data);
void __position_updated_cb(double latitude, double longitude, double altitude, time_t timestamp, void *user_data);
void __state_changed_cb(location_service_state_e state, void *user_data);

void __batch_updated_cb(int num_of_location, void *user_data);
bool __get_batch_cb(double latitude, double longitude, double altitude, double speed, double direction, double horizontal, double vertical, time_t timestamp, void *user_data);
void __zone_changed_cb(location_boundary_state_e state, double latitude, double longitude, double altitude, time_t timestamp, void *user_data);
void __setting_changed_cb(location_method_e method, bool enable, void *user_data);

#endif /* __LBS_SETTING_TEST_H__ */

