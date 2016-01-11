/* -*- mode: C; c-file-style: "gnu"; indent-tabs-mode: nil; -*- */
/*
 * Copyright (C) 2011 Red Hat
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License as
 * published by the Free Software Foundation; either version 2 of the
 * License, or (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful, but
 * WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place - Suite 330, Boston, MA
 * 02111-1307, USA.
 *
 * Authors:
 *      Rémi Cohen-Scali <rcoscali@gmail.com>
 */

#include <stdio.h>
#include <stddef.h>
#include <stdlib.h>
#include <string.h>

#include <glib.h>
#include <gio/gio.h>
#include <json-glib/json-glib.h>

#include "ppapi/c/pp_completion_callback.h"
#include "ppapi/c/pp_errors.h"
#include "ppapi/c/pp_instance.h"
#include "ppapi/c/pp_module.h"
#include "ppapi/c/pp_size.h"
#include "ppapi/c/pp_var.h"
#include "ppapi/c/pp_input_event.h"
#include "ppapi/c/ppb.h"
#include "ppapi/c/ppb_core.h"
#include "ppapi/c/ppb_graphics_2d.h"
#include "ppapi/c/ppb_image_data.h"
#include "ppapi/c/ppb_input_event.h"
#include "ppapi/c/ppb_instance.h"
#include "ppapi/c/ppp.h"
#include "ppapi/c/ppp_instance.h"

struct PP_InputEvent;

PP_Module g_module_id;
PPB_GetInterface g_get_browser_interface = NULL;

const struct PPB_Instance* g_instance_interface;

typedef unsigned char bool;
static bool true = (0 == 0);
static bool false = (0 == 1);

// PPP_Instance implementation -------------------------------------------------
struct InstanceInfo 
{
  PP_Instance pp_instance;
  GDBusProxy *proxy;
  
  struct InstanceInfo* next;
};

typedef struct InstanceInfo InstanceInfo;

// Linked list of all live instances.
InstanceInfo* all_instances = (InstanceInfo*)NULL;

static inline gchar *
get_string_property (InstanceInfo *instance,
                     NPObject     *obj,
                     const char *name)
{
  NPVariant result = { NPVariantType_Void };
  NPString result_str;
  gchar *result_copy;

  result_copy = NULL;

  if (!funcs.getproperty (instance, obj,
                          funcs.getstringidentifier (name),
                          &result))
    goto out;

  if (!NPVARIANT_IS_STRING (result))
    goto out;

  result_str = NPVARIANT_TO_STRING (result);
  result_copy = g_strndup (result_str.UTF8Characters, result_str.UTF8Length);

 out:
  funcs.releasevariantvalue (&result);
  return result_copy;
}

static gboolean
check_origin_and_protocol (NPP instance)
{
  gboolean ret = FALSE;
  NPError error;
  NPObject *window = NULL;
  NPVariant document = { NPVariantType_Void };
  NPVariant location = { NPVariantType_Void };
  gchar *hostname = NULL;
  gchar *protocol = NULL;

  error = funcs.getvalue (instance, NPNVWindowNPObject, &window);
  if (error != NPERR_NO_ERROR)
    goto out;

  if (!funcs.getproperty (instance, window,
                          funcs.getstringidentifier ("document"),
                          &document))
    goto out;

  if (!NPVARIANT_IS_OBJECT (document))
    goto out;

  if (!funcs.getproperty (instance, NPVARIANT_TO_OBJECT (document),
                          funcs.getstringidentifier ("location"),
                          &location))
    goto out;

  if (!NPVARIANT_IS_OBJECT (location))
    goto out;

  hostname = get_string_property (instance,
                                  NPVARIANT_TO_OBJECT (location),
                                  "hostname");

  if (g_strcmp0 (hostname, ORIGIN))
    {
      g_debug ("origin does not match, is %s",
               hostname);

      goto out;
    }

  protocol = get_string_property (instance,
                                  NPVARIANT_TO_OBJECT (location),
                                  "protocol");

  if (g_strcmp0 (protocol, "https:") != 0)
    {
      g_debug ("protocol does not match, is %s",
               protocol);

      goto out;
    }

  ret = TRUE;

 out:
  g_free (protocol);
  g_free (hostname);

  funcs.releasevariantvalue (&location);
  funcs.releasevariantvalue (&document);

  if (window != NULL)
    funcs.releaseobject (window);
  return ret;
}

/*
 * Returns the info for the given instance, or NULL if it's not found.
 */
InstanceInfo* 
FindInstance(PP_Instance instance) 
{
  InstanceInfo* cur = all_instances;

  while (cur) 
    if (cur->pp_instance == instance)
      return (cur);

  return (NULL);
}

/*
 * Create a new plugin instance
 */
bool 
Instance_New(PP_Instance instance) 
{
  InstanceInfo* info = (InstanceInfo*)malloc(sizeof(InstanceInfo));
  if (!info)
    {
      fprintf(stderr, "Error: Instance allocation failed\n");
      return (false);
    }
  info->pp_instance = instance;
  
  fprintf(stderr, "Instance_New: plugin created\n");

  // Insert into linked list of live instances.
  info->next = all_instances;
  all_instances = info;

  info->proxy = g_dbus_proxy_new_for_bus_sync (G_BUS_TYPE_SESSION,
                                               G_DBUS_PROXY_FLAGS_NONE,
                                               NULL, /* interface info */
                                               "org.gnome.Shell",
                                               "/org/gnome/Shell",
                                               "org.gnome.Shell.Extensions",
                                               NULL, /* GCancellable */
                                               &error);
  if (!data->proxy)
    {
      /* ignore error if the shell is not running, otherwise warn */
      if (error->domain != G_DBUS_ERROR ||
          error->code != G_DBUS_ERROR_NAME_HAS_NO_OWNER)
        {
          g_warning ("Failed to set up Shell proxy: %s", error->message);
        }
      g_clear_error (&error);
      return (false);
    }

  return (true);
}

/*
 * Delete an instance
 */
void 
Instance_Delete(PP_Instance instance) 
{
  // Find the matching item in the linked list, delete it, and patch the links.
  InstanceInfo** prev_ptr = &all_instances;
  InstanceInfo* cur = all_instances;

  while (cur) 
    {
      if (instance == cur->pp_instance) 
	{
	  *prev_ptr = cur->next;
	  free(cur);
	  return;
	}
      prev_ptr = &cur->next;
    }
}

/*
 * Initialize an instance
 */
bool 
Instance_Initialize(PP_Instance pp_instance,
		    uint32_t argc,
		    const char* argn[],
		    const char* argv[]) 
{
  return (true);
}

/*
 * Handle document load
 */
bool 
Instance_HandleDocumentLoad(PP_Instance pp_instance,
			    PP_Resource pp_url_loader) 
{
  return (false);
}

/*
 * Handle input event
 */
bool 
Instance_HandleInputEvent(PP_Instance pp_instance,
			  const struct PP_InputEvent* event) 
{
  // We don't handle any events.
  return (false);
}

/*
 * Handle focus changed
 */
void 
Instance_HandleFocusChanged(bool __attribute__((unused)) has_focus) 
{
}

/*
 *
 */
struct PP_Var Instance_GetInstanceObject(PP_Instance pp_instance) {
  return PP_MakeNull();
}

static PPP_Instance instance_interface = {
  &Instance_New,
  &Instance_Delete,
  &Instance_Initialize,
  &Instance_HandleDocumentLoad,
  &Instance_HandleInputEvent,
  &Instance_HandleFocusChanged,
  &Instance_GetInstanceObject,
  NULL,
  NULL
};

/*
 * Initialize module
 */
PP_EXPORT int32_t 
PPP_InitializeModule(PP_Module module_id,
		     PPB_GetInterface get_browser_interface) 
{
  // Save the global module information for later.
  g_module_id = module_id;
  g_get_browser_interface = get_browser_interface;

  printf("[ppapi-plugin.c] PPP_InitializeModule(): Initialization OK\n");
  
  return PP_OK;
}

/*
 * Shutdown module
 */
PP_EXPORT void 
PPP_ShutdownModule(void) 
{
  printf("[ppapi-plugin.c] PPP_ShutdownModule(): Shutdown OK\n");
}

/*
 * Retrieve instance interface by name
 */
PP_EXPORT const void* 
PPP_GetInterface(const char* interface_name) 
{
  // You will normally implement a getter for at least PPP_INSTANCE_INTERFACE
  // here.
  printf("[ppapi-plugin.c] PPP_GetInterface(): Returning NULL interface\n");
  return NULL;
}
