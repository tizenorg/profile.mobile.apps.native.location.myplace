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
#include <dlog.h>
#include <Elementary.h>
#include <ui-gadget.h>
#include <vconf.h>
#include <libintl.h>

#include "myplace-common.h"

static void timeout_cb(void *data, Evas_Object *obj, void *event_info)
{
	evas_object_del(data);
}

Evas_Object *create_popup(Evas_Object *parent, char *style, char *text)
{
	Evas_Object *popup;

	popup = elm_popup_add(parent);
	if (style)
		elm_object_style_set(popup, style);
	if (text)
		elm_object_text_set(popup, text);
	evas_object_show(popup);

	return popup;
}

void popup_back_cb(void *data, Evas_Object *obj, void *event_info)
{
	evas_object_del(obj);
	if (data) {
		Evas_Object *popup = (Evas_Object *)data;
		LS_RETURN_IF_FAILED(popup);
		popup = NULL;
	}
}

void toast_popup(myplace_app_data *ad, char *str)
{
	Evas_Object *popup;

	popup = create_popup(ad->nf, "toast", str);
	evas_object_size_hint_weight_set(popup, EVAS_HINT_EXPAND, EVAS_HINT_EXPAND);
	elm_popup_timeout_set(popup, 2.0);
	evas_object_smart_callback_add(popup, "timeout", timeout_cb, NULL);

	evas_object_show(popup);
}
