/* Contains a few utility function used internally by the GtkScalableImage implementation */

typedef struct _GtkRequisition Size;

struct _GtkScalableImagePrivate
{
	void* placeholder;
};

enum
{
	PROP_HADJUSTMENT = 1,
	PROP_VADJUSTMENT,
	PROP_HSCROLL_POLICY,
	PROP_VSCROLL_POLICY,
};


/* Returns the widget size needed to display the whole image at the current scale.
 * For example an 800x600 image needs 400x300 pixels when scaled to 0.5. */
static
Size
_gtk_scalable_image_get_minimum_size(GtkScalableImage* self)
{
	g_assert(self->scale > 0.0);
	Size result = { 0, 0 };
	if(self->pixbuf)
	{
		result.width  = (gint)(self->scale * gdk_pixbuf_get_width(self->pixbuf));
		result.height = (gint)(self->scale * gdk_pixbuf_get_height(self->pixbuf));
	}
	return result;
}


/* Returns the widget size needed to display the whole unscaled image.
 * For example an 800x600 image scaled to 0.5 still needs 800x600 pixels to be displayed at its natural size. */
static
Size
_gtk_scalable_image_get_natural_size(GtkScalableImage* self)
{
	Size result = { 0, 0 };
	if(self->pixbuf)
	{
		result.width  = gdk_pixbuf_get_width(self->pixbuf);
		result.height = gdk_pixbuf_get_height(self->pixbuf);
	}
	return result;
}


static
Size
_gtk_scalable_image_get_allocated_size(GtkScalableImage* self)
{
	GtkAllocation allocation;
	gtk_widget_get_allocation(GTK_WIDGET(self), &allocation);
	Size result = { allocation.width, allocation.height };
	return result;
}


static
void
_gtk_scalable_image_update_viewport_size(GtkScalableImage* self, Size* allocation_size)
{
	g_assert(self->scale > 0.0);
	self->viewport.width  = allocation_size->width  / self->scale;
	self->viewport.height = allocation_size->height / self->scale;
}


/* Centers the image if the viewport is larger than the image. 
 * Otherwise forces the viewport to be inside the image rectangle */
static
void
_gtk_scalable_image_adjust_viewport_position(GtkScalableImage* self)
{
	Size image_size = _gtk_scalable_image_get_natural_size(self);

	if(self->viewport.width > image_size.width)
		self->viewport.x = -(self->viewport.width - image_size.width) / 2;
	else
		CLAMP(self->viewport.x, 0, image_size.width - self->viewport.width);
	
	if(self->viewport.height > image_size.height)
		self->viewport.y = -(self->viewport.height - image_size.height) / 2;
	else
		CLAMP(self->viewport.y, 0, image_size.height - self->viewport.height);
}


/* Synchronizes the adjustments with the viewport. Called when:
 * -- the pixbuf changes            (changes image size    -> changes upper limit)
 * -- the widget allocation changes (changes viewport size -> changes page size)
 * -- the scale changes             (changes viewport size -> changes page size) */
static
void
_gtk_scalable_image_reset_adjustments(GtkScalableImage* self)
{
	g_assert(self->hadjustment);
	g_assert(self->vadjustment);

	gdouble value[2]     = { 0.0, 0.0 };
	gdouble upper[2]     = { 0.0, 0.0 };
	gdouble page_size[2] = { 0.0, 0.0 };

	if(self->pixbuf)
	{
		Size image_size = _gtk_scalable_image_get_natural_size(self);
		value[0] = (gdouble)CLAMP(self->viewport.x, 0, image_size.width - self->viewport.width);
		value[1] = (gdouble)CLAMP(self->viewport.y, 0, image_size.height - self->viewport.height);
		
		upper[0] = (gdouble)image_size.width;
		upper[1] = (gdouble)image_size.height;
		
		page_size[0] = (gdouble)self->viewport.width;
		page_size[1] = (gdouble)self->viewport.height;
	}

	gtk_adjustment_configure(self->hadjustment,
	                         value[0],
	                         gtk_adjustment_get_lower(self->hadjustment),
	                         upper[0],
	                         gtk_adjustment_get_step_increment(self->hadjustment),
	                         page_size[0] * 0.5,
	                         page_size[0]);
	gtk_adjustment_configure(self->vadjustment,
	                         value[1],
	                         gtk_adjustment_get_lower(self->vadjustment),
	                         upper[1],
	                         gtk_adjustment_get_step_increment(self->vadjustment),
	                         page_size[1] * 0.5,
	                         page_size[1]);
}


static
void gtk_scalable_image_on_signal_adjustment_value_changed(GtkAdjustment*, GtkScalableImage*);

/* Convenience function used by gtk_scalable_image_set_property */
static
void
_gtk_scalable_image_set_adjustment(GtkScalableImage* self,
                                   GtkAdjustment**   old_adjustment,
                                   GtkAdjustment*    new_adjustment)
{
	if(new_adjustment && *old_adjustment != new_adjustment)
	{
		if(*old_adjustment)
		{
			gint disconnected_count =
			g_signal_handlers_disconnect_by_func(*old_adjustment,
			                                     gtk_scalable_image_on_signal_adjustment_value_changed,
			                                     self);
			g_object_unref(*old_adjustment);
			g_assert(disconnected_count == 1);
		}
		*old_adjustment = new_adjustment;
		g_object_ref_sink(new_adjustment);
		g_signal_connect(new_adjustment, "value-changed",
		                 G_CALLBACK(gtk_scalable_image_on_signal_adjustment_value_changed), self);
		gtk_scalable_image_on_signal_adjustment_value_changed(new_adjustment, self);
	}
}