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

#ifndef MYPLACE_PLACELIST_H_
#define MYPLACE_PLACELIST_H_

Evas_Object *create_indicator_bg(Evas_Object * parent);
Evas_Object *create_bg(Evas_Object * parent);
Evas_Object *create_conformant(Evas_Object * parent);
Evas_Object *create_layout(Evas_Object * parent);
void profile_changed_cb(void *data, Evas_Object * obj, void *event);
void win_del(void *data, Evas_Object * obj, void *event);
Evas_Object *create_win(const char *name);

int myplace_geofence_init(myplace_app_data *ad);
int myplace_geofence_deinit( myplace_app_data *ad);

char *myplace_place_text_get(void *data, Evas_Object *obj, const char *part);
void myplace_placelist_cb(void *data);

#endif /* MYPLACE_PLACELIST_H_ */
