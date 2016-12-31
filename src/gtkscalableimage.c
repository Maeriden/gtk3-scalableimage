#include "gtkscalableimage.h"
#include "gtkscalableimage-private.c"


G_DEFINE_TYPE_WITH_CODE(GtkScalableImage, gtk_scalable_image, GTK_TYPE_WIDGET,
                        G_ADD_PRIVATE(GtkScalableImage)
                        G_IMPLEMENT_INTERFACE(GTK_TYPE_SCROLLABLE, NULL));


double
gtk_scalable_image_get_scale(GtkScalableImage* self)
{
	g_return_val_if_fail(GTK_IS_SCALABLE_IMAGE(self), 0.0);
	return self->scale;
}


void
gtk_scalable_image_set_scale(GtkScalableImage* self, double scale)
{
	g_return_if_fail(GTK_IS_SCALABLE_IMAGE(self));

	if(scale > 0.0)
	{
		self->is_fitting = FALSE;
		self->scale = scale;

		Size allocation_size = _gtk_scalable_image_get_allocated_size(self);
		_gtk_scalable_image_update_viewport_size(self, &allocation_size);
		_gtk_scalable_image_adjust_viewport_position(self);
		_gtk_scalable_image_reset_adjustments(self);
		
		gtk_widget_queue_draw(GTK_WIDGET(self));
	}
}


/* Computing this was hell because I suck at math 
 * http://stackoverflow.com/questions/2916081/zoom-in-on-a-point-using-scale-and-translate */
void
gtk_scalable_imagevv_set_scale_at_point(GtkScalableImage* self, double scale, gint x_vspace, gint y_vspace)
{
	g_return_if_fail(GTK_IS_SCALABLE_IMAGE(self));

	if(scale > 0.0)
	{
		gint old_image_x = (x_vspace / self->scale);
		gint old_image_y = (y_vspace / self->scale);
		gint new_image_x = (x_vspace / scale);
		gint new_image_y = (y_vspace / scale);

		self->is_fitting = FALSE;
		self->scale = scale;
		Size allocation_size = _gtk_scalable_image_get_allocated_size(self);
		_gtk_scalable_image_update_viewport_size(self, &allocation_size);
		
		self->viewport.x += old_image_x - new_image_x;
		self->viewport.y += old_image_y - new_image_y;

		_gtk_scalable_image_adjust_viewport_position(self);
		_gtk_scalable_image_reset_adjustments(self);
		
		gtk_widget_queue_draw(GTK_WIDGET(self));
	}
}


void
gtk_scalable_image_set_scale_to_fit(GtkScalableImage* self)
{
	g_return_if_fail(GTK_IS_SCALABLE_IMAGE(self));
	
	self->is_fitting = TRUE;
	if(self->pixbuf)
	{
		// TODO: Compute appropriate scale and assign to self
		Size image_natural_size = _gtk_scalable_image_get_natural_size(self);
		Size allocated_size     = _gtk_scalable_image_get_allocated_size(self);
		
		double ratio_x = (double)allocated_size.width  / (double)image_natural_size.width;
		double ratio_y = (double)allocated_size.height / (double)image_natural_size.height;
		double scale   = MIN(ratio_x, ratio_y);
		if(scale > 0.0)
		{
			self->scale = scale;
			_gtk_scalable_image_update_viewport_size(self, &allocated_size);
			_gtk_scalable_image_adjust_viewport_position(self);

			_gtk_scalable_image_reset_adjustments(self);
			gtk_widget_queue_draw(GTK_WIDGET(self));
		}
	}
}

GdkPixbuf*
gtk_scalable_image_get_pixbuf(GtkScalableImage* self)
{
	g_return_val_if_fail(GTK_IS_SCALABLE_IMAGE(self), NULL);
	return self->pixbuf;
}


void
gtk_scalable_image_set_pixbuf(GtkScalableImage* self, GdkPixbuf* pixbuf)
{
	g_return_if_fail(GTK_IS_SCALABLE_IMAGE(self));

	if(self->pixbuf != pixbuf)
	{
		if(self->pixbuf)
			g_object_unref(self->pixbuf);
		self->pixbuf = pixbuf;
		
		if(self->pixbuf)
		{
			g_object_ref(self->pixbuf);
			// TODO: Reset viewport maybe?
			
			// FIXME: This check avoids a useless call to _gtk_scalable_image_update_adjustments() when the widget
			//        is constructed from gtk_scalable_image_new_with_pixbuf(). At contruction time the widget
			//        does not know its size allocation, so it cannot set the page size.
			//        We use gtk_widget_get_realized() because I do not know a better alternative
			if(gtk_widget_get_realized(GTK_WIDGET(self)))
			{
				_gtk_scalable_image_reset_adjustments(self);
			}
		}
		else
		{
			// TODO: Reset viewport maybe?
			if(gtk_widget_get_realized(GTK_WIDGET(self)))
			{
				_gtk_scalable_image_reset_adjustments(self);
			}
		}
		
		gtk_widget_queue_draw(GTK_WIDGET(self));
	}
}


void
gtk_scalable_image_translate(GtkScalableImage* self, gint delta_x, gint delta_y)
{
	g_return_if_fail(GTK_IS_SCALABLE_IMAGE(self));
	
	if(self->pixbuf)
	{
		Size image_size  = _gtk_scalable_image_get_natural_size(self);

		// TODO: Check if the viewport's' upper is synchronized with the adjustment's upper
		if(self->viewport.width < image_size.width)
		{
			self->viewport.x = CLAMP(self->viewport.x + delta_x, 0, image_size.width  - self->viewport.width);
			// g_signal_handler_block(self->hadjustment, "value-changed");
			gtk_adjustment_set_value(self->hadjustment, (gdouble)self->viewport.x);
			// g_signal_handler_unblock(self->hadjustment, "value-changed");
		}
		else
		{
			_gtk_scalable_image_adjust_viewport_position(self);
		}

		if(self->viewport.height < image_size.height)
		{
			self->viewport.y = CLAMP(self->viewport.y + delta_y, 0, image_size.height - self->viewport.height);
			// g_signal_handler_block(self->vadjustment, "value-changed");
			gtk_adjustment_set_value(self->vadjustment, (gdouble)self->viewport.y);
			// g_signal_handler_unblock(self->vadjustment, "value-changed");
		}
		else
		{
			_gtk_scalable_image_adjust_viewport_position(self);
		}
		
		gtk_widget_queue_draw(GTK_WIDGET(self));
	}
}


static
void
gtk_scalable_image_on_signal_adjustment_value_changed(GtkAdjustment*    adjustment,
                                                      GtkScalableImage* self)
{
	if(adjustment == self->hadjustment)
	{
		self->viewport.x = (gint)gtk_adjustment_get_value(adjustment);
		_gtk_scalable_image_adjust_viewport_position(self);
		// Size image_size = _gtk_scalable_image_get_natural_size(self);
		// if(self->viewport.width < image_size.width)
		// 	self->viewport.x = (gint)gtk_adjustment_get_value(adjustment);
		// else
		// 	_gtk_scalable_image_adjust_viewport_position(self);
		
		gtk_widget_queue_draw(GTK_WIDGET(self));
		return;
	}

	if(adjustment == self->vadjustment)
	{
		self->viewport.y = (gint)gtk_adjustment_get_value(adjustment);
		_gtk_scalable_image_adjust_viewport_position(self);
		// Size image_size = _gtk_scalable_image_get_natural_size(self);
		// if(self->viewport.height < image_size.height)
		// 	self->viewport.y = (gint)gtk_adjustment_get_value(adjustment);
		// else
		// 	_gtk_scalable_image_adjust_viewport_position(self);
		
		gtk_widget_queue_draw(GTK_WIDGET(self));
		return;
	}
}


/* Realization is the process of creating GDK resources associated with the widget; including but not limited to widget->window and widget->style.
 * A realize method should do the following:
 * -- Set the GTK_REALIZED flag.
 * -- Create the widget's windows, especially widget->window which should be a child of the
 *      parent's widget->window (obtained with gtk_widget_get_parent_window()).
 * -- Place a pointer to the widget in the user data field of each window.
 * -- For windowless widgets, widget->window should be set to the parent widget's window
 *      (obtained with gtk_widget_get_parent_window()). These widgets should also increase
 *      the reference count on widget->window by calling gdk_window_ref().
 * -- Set widget->style using gtk_style_attach().
 * -- Set the background of each window using gtk_style_set_background() if possible, and
 *      failing that using some color from the style.
 *      A windowless widget should not do this, since its parent already has.
 *
 * The default implementation of realize is only appropriate for windowless widgets;
 * it sets the GTK_REALIZED flag, sets widget->window to the parent widget's window,
 * increases the reference count on the parent's window, and sets widget->style.
 * Widgets with their own GdkWindow must override the realize method.
 * The "realize" signal invokes the realize method as its default handler.
 * This signal should never be emitted directly, because there are substantial
 * pre- and post-conditions to be enforced. gtk_widget_realize() takes care of the details.
 * Among other things, it ensures that the widget's parent is realized;
 * GTK+ maintains the invariant that widgets cannot be realized unless their parents are also realized */
static
void
gtk_scalable_image_realize(GtkWidget* widget)
{
	GtkScalableImage* self = GTK_SCALABLE_IMAGE(widget);

	GtkAllocation allocation;
	gtk_widget_get_allocation(widget, &allocation);
	GdkWindowAttr window_attrs;
	window_attrs.window_type = GDK_WINDOW_CHILD;
	window_attrs.x           = allocation.x;
	window_attrs.y           = allocation.y;
	window_attrs.width       = allocation.width;
	window_attrs.height      = allocation.height;
	window_attrs.wclass      = GDK_INPUT_OUTPUT;
	window_attrs.visual      = gtk_widget_get_visual (widget);
	// window_attrs.colormap = gtk_widget_get_colormap(widget);
	window_attrs.event_mask  = gtk_widget_get_events(widget) |
	                           GDK_EXPOSURE_MASK             |
	                           GDK_BUTTON_MOTION_MASK        |
	                           GDK_BUTTON_PRESS_MASK         |
	                           GDK_BUTTON_RELEASE_MASK       |
	                           GDK_POINTER_MOTION_MASK;

	// int attr_mask = GDK_WA_X | GDK_WA_Y | GDK_WA_VISUAL | GDK_WA_COLORMAP;
	int attr_mask = GDK_WA_X | GDK_WA_Y | GDK_WA_VISUAL;
	GdkWindow* parent = gtk_widget_get_parent_window(widget);
	GdkWindow* window = gdk_window_new(parent, &window_attrs, attr_mask);
	gtk_widget_set_window(widget, window);
	gdk_window_set_user_data(window, self);

	// TODO
	// GtkStyleContext* style = gtk_widget_get_style_context(widget);
	// style = gtk_style_attach(style, window);
	// gtk_widget_set_style(widget, style);
	// gtk_style_set_background(style, window, GTK_STATE_NORMAL);

	gtk_widget_set_realized(widget, TRUE);
}


static
void
gtk_scalable_image_get_preferred_height(GtkWidget* widget,
                                        gint*      minimum_size,
                                        gint*      natural_size)
{
	GtkScalableImage* self = GTK_SCALABLE_IMAGE(widget);
	if(minimum_size)
		*minimum_size = _gtk_scalable_image_get_minimum_size(self).height;
	if(natural_size)
		*natural_size = _gtk_scalable_image_get_natural_size(self).height;
}


static
void
gtk_scalable_image_get_preferred_width(GtkWidget* widget,
                                       gint*      minimum_size,
                                       gint*      natural_size)
{
	GtkScalableImage* self = GTK_SCALABLE_IMAGE(widget);
	if(minimum_size)
		*minimum_size = _gtk_scalable_image_get_minimum_size(self).width;
	if(natural_size)
		*natural_size = _gtk_scalable_image_get_natural_size(self).width;
}


/* Called at the end of gtk_widget_size_allocate() after adjustment has been performed.
 * The default implementation (gtk_widget_real_size_allocate) calls gtk_widget_set_allocation() to
 * store the allocation into the widget private data and then raises the size-allocate signal. */
static
void
gtk_scalable_image_size_allocate(GtkWidget* widget, GtkAllocation* allocation)
{
	GtkScalableImage* self = GTK_SCALABLE_IMAGE(widget);

	Size allocation_size = { allocation->width, allocation->height };
	_gtk_scalable_image_update_viewport_size(self, &allocation_size);

	GTK_WIDGET_CLASS(gtk_scalable_image_parent_class)->size_allocate(widget, allocation);

	// g_debug("gtk_scalable_image_size_allocate(allocated = (%d, %d), viewport = (%d, %d))",
	//         allocation->width, allocation->height, self->viewport.width, self->viewport.height);

	if(self->is_fitting)
	{
		gtk_scalable_image_set_scale_to_fit(self);

		// Called by gtk_scalable_image_set_scale_to_fit()
		// _gtk_scalable_image_maybe_center_image
		// _gtk_scalable_image_configure_adjustments(self);
	}
	else
	{
		_gtk_scalable_image_adjust_viewport_position(self);
		_gtk_scalable_image_reset_adjustments(self);
	}
}


/* GTK3 removed expose-event, so now all the drawing is done in draw() */
static
gboolean
gtk_scalable_image_draw(GtkWidget* widget, cairo_t* context)
{
	// g_debug("gtk_scalable_image_draw");
	// g_debug("------------------------------------------------------------");
	GtkScalableImage* self = GTK_SCALABLE_IMAGE(widget);

	if(!self->pixbuf)
		return FALSE;

	cairo_save(context);
	cairo_scale(context, self->scale, self->scale);
	cairo_translate(context, -self->viewport.x, -self->viewport.y);
	gdk_cairo_set_source_pixbuf(context, self->pixbuf, 0.0, 0.0);
	cairo_paint(context);
	cairo_restore(context);
	return TRUE;
}


static
void
gtk_scalable_image_get_property(GObject*    object,
                                guint       property_id,
                                GValue*     value,
                                GParamSpec* param_spec)
{
	GtkScalableImage* self = GTK_SCALABLE_IMAGE(object);
	switch(property_id)
	{
		case PROP_HADJUSTMENT:
		{
			g_value_set_object(value, self->hadjustment);
		} break;

		case PROP_VADJUSTMENT:
		{
			g_value_set_object(value, self->vadjustment);
		} break;

		case PROP_HSCROLL_POLICY:
		{
			g_value_set_enum(value, self->hscroll_policy);
		} break;

		case PROP_VSCROLL_POLICY:
		{
			g_value_set_enum(value, self->vscroll_policy);
		} break;
		
		default:
		{
			G_OBJECT_WARN_INVALID_PROPERTY_ID(object, property_id, param_spec);
		} break;
	}
}


static
void
gtk_scalable_image_set_property(GObject*      object,
                                guint         property_id,
                                const GValue* value,
                                GParamSpec*   param_spec)
{
	GtkScalableImage* self = GTK_SCALABLE_IMAGE(object);
	switch(property_id)
	{
		case PROP_HADJUSTMENT:
		{
			GtkAdjustment* adjustment = GTK_ADJUSTMENT(g_value_get_object(value));
			_gtk_scalable_image_set_adjustment(self, &self->hadjustment, adjustment);
		} break;

		case PROP_VADJUSTMENT:
		{
			GtkAdjustment* adjustment = GTK_ADJUSTMENT(g_value_get_object(value));
			_gtk_scalable_image_set_adjustment(self, &self->vadjustment, adjustment);
		} break;

		case PROP_HSCROLL_POLICY:
		{
			gint scroll_policy = g_value_get_enum(value);
			if(scroll_policy != self->hscroll_policy)
			{
				self->hscroll_policy = scroll_policy;
				// gtk_widget_queue_resize(GTK_WIDGET(self));
				// g_object_notify_by_pspec(object, param_spec);
			}
		} break;

		case PROP_VSCROLL_POLICY:
		{
			gint scroll_policy = g_value_get_enum(value);
			if(scroll_policy != self->vscroll_policy)
			{
				self->vscroll_policy = scroll_policy;
				// gtk_widget_queue_resize(GTK_WIDGET(self));
				// g_object_notify_by_pspec(object, param_spec);
			}
		} break;
		
		default:
		{
			G_OBJECT_WARN_INVALID_PROPERTY_ID(object, property_id, param_spec);
		} break;
	}
}


static
void
gtk_scalable_image_finalize(GObject* object)
{
	GtkScalableImage* self = GTK_SCALABLE_IMAGE(object);
	if(self->hadjustment)
	{
		g_signal_handlers_disconnect_by_data(G_OBJECT(self->hadjustment), self);
		g_object_unref(self->hadjustment);
		self->hadjustment = NULL;
	}
	if(self->vadjustment)
	{
		g_signal_handlers_disconnect_by_data(G_OBJECT(self->vadjustment), self);
		g_object_unref(self->vadjustment);
		self->vadjustment = NULL;
	}
	if(self->pixbuf)
	{
		g_object_unref(self->pixbuf);
		self->pixbuf = NULL;
	}
	
	G_OBJECT_CLASS(gtk_scalable_image_parent_class)->finalize(object);
}


static
void
gtk_scalable_image_init(GtkScalableImage* self)
{
	self->pixbuf         = NULL;
	self->viewport       = (GdkRectangle) { 0, 0, 0, 0 };
	self->scale          = 1.0;
	self->is_fitting     = TRUE;
	self->hscroll_policy = GTK_SCROLL_MINIMUM;
	self->vscroll_policy = GTK_SCROLL_MINIMUM;

	// self->hadjustment = GTK_ADJUSTMENT(gtk_adjustment_new(0.0, 0.0, 0.0, 0.0, 0.0, 0.0));
	// g_object_ref(self->hadjustment);
	// g_object_ref_sink(self->hadjustment);
	// self->vadjustment = GTK_ADJUSTMENT(gtk_adjustment_new(0.0, 0.0, 0.0, 0.0, 0.0, 0.0));
	// g_object_ref(self->vadjustment);
	// g_object_ref_sink(self->vadjustment);

	GdkEventMask events = GDK_BUTTON_PRESS_MASK | GDK_BUTTON_RELEASE_MASK |
	                      GDK_KEY_PRESS_MASK | GDK_KEY_RELEASE_MASK |
	                      GDK_SCROLL_MASK;
	gtk_widget_add_events(GTK_WIDGET(self), events);

	// g_signal_connect(self->hadjustment, "value_changed",
	//                  G_CALLBACK(gtk_scalable_image_on_signal_adjustment_value_changed), self);
	// g_signal_connect(self->vadjustment, "value_changed",
	//                  G_CALLBACK(gtk_scalable_image_on_signal_adjustment_value_changed), self);
	
	// gtk_widget_set_can_focus(GTK_WIDGET(self), TRUE);
}


static
void
gtk_scalable_image_class_init(GtkScalableImageClass* klass)
{
    GtkWidgetClass* widget_class = GTK_WIDGET_CLASS(klass);
    widget_class->realize              = gtk_scalable_image_realize;
    widget_class->get_preferred_height = gtk_scalable_image_get_preferred_height;
    widget_class->get_preferred_width  = gtk_scalable_image_get_preferred_width;
    widget_class->size_allocate        = gtk_scalable_image_size_allocate;
    widget_class->draw                 = gtk_scalable_image_draw;

	GObjectClass* gobject_class = G_OBJECT_CLASS(klass);
	gobject_class->get_property = gtk_scalable_image_get_property;
	gobject_class->set_property = gtk_scalable_image_set_property;
	gobject_class->finalize     = gtk_scalable_image_finalize;

	/* GtkScrollable implementation */
	g_object_class_override_property(gobject_class, PROP_HADJUSTMENT,    "hadjustment");
	g_object_class_override_property(gobject_class, PROP_VADJUSTMENT,    "vadjustment");
	g_object_class_override_property(gobject_class, PROP_HSCROLL_POLICY, "hscroll-policy");
	g_object_class_override_property(gobject_class, PROP_VSCROLL_POLICY, "vscroll-policy");

	// gtk_widget_class_set_css_name(widget_class, "scrollableimage");
}


GtkWidget*
gtk_scalable_image_new()
{
	GtkWidget* self = gtk_widget_new(GTK_SCALABLE_IMAGE_TYPE,
	                                 "hadjustment", gtk_adjustment_new(0.0, 0.0, 0.0, 20.0, 0.0, 0.0),
	                                 "vadjustment", gtk_adjustment_new(0.0, 0.0, 0.0, 20.0, 0.0, 0.0),
	                                 NULL);
	return self;
}


GtkWidget*
gtk_scalable_image_new_from_pixbuf(GdkPixbuf* pixbuf)
{
	GtkWidget* self = gtk_widget_new(GTK_SCALABLE_IMAGE_TYPE,
	                                 "hadjustment", gtk_adjustment_new(0.0, 0.0, 0.0, 20.0, 0.0, 0.0),
	                                 "vadjustment", gtk_adjustment_new(0.0, 0.0, 0.0, 20.0, 0.0, 0.0),
	                                 NULL);
	if(pixbuf)
	{
		gtk_scalable_image_set_pixbuf(GTK_SCALABLE_IMAGE(self), pixbuf);
	}
	return self;
}
