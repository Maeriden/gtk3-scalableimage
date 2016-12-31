#ifndef _GTK_SCALABLE_IMAGE_H
#define _GTK_SCALABLE_IMAGE_H

#include <gdk/gdk.h>
#include <gtk/gtk.h>


G_BEGIN_DECLS

#define GTK_SCALABLE_IMAGE_TYPE            (gtk_scalable_image_get_type())
#define GTK_SCALABLE_IMAGE(obj)            (G_TYPE_CHECK_INSTANCE_CAST((obj), GTK_SCALABLE_IMAGE_TYPE, GtkScalableImage))
#define GTK_SCALABLE_IMAGE_CLASS(klass)    (G_TYPE_CHECK_CLASS_CAST((klass),  GTK_SCALABLE_IMAGE_TYPE, GtkScalableImageClass))
#define GTK_IS_SCALABLE_IMAGE(obj)         (G_TYPE_CHECK_INSTANCE_TYPE((obj), GTK_SCALABLE_IMAGE_TYPE))
#define GTK_IS_SCALABLE_IMAGE_CLASS(klass) (G_TYPE_CHECK_CLASS_TYPE((klass),  GTK_SCALABLE_IMAGE_TYPE))
#define GTK_SCALABLE_IMAGE_GET_CLASS(obj)  (G_TYPE_INSTANCE_GET_CLASS((obj),  GTK_SCALABLE_IMAGE_TYPE, GtkScalableImageClass))


typedef struct _GtkScalableImage            GtkScalableImage;
typedef struct _GtkScalableImagePrivate     GtkScalableImagePrivate;
typedef struct _GtkScalableImageClass       GtkScalableImageClass;

// NOTE: The viewport uses the image's coordinate system
// FIXME: In fit-to-window mode the size_allocate() function is called twice,
// presumably due to the scrollbars taking up allocation space
struct _GtkScalableImage
{
	GtkWidget base;
	
	GdkPixbuf*          pixbuf;
	GdkRectangle        viewport;
	
	/* Adjustments of the scrollable widget are shared between the scrollable widget and its parent. */
	GtkAdjustment*      hadjustment;
	GtkAdjustment*      vadjustment;
	
	/* Determines whether scrolling should start once the scrollable widget is
	 * allocated less than its minimum size or less than its natural size. */
	GtkScrollablePolicy hscroll_policy;
	GtkScrollablePolicy vscroll_policy;
	
	double              scale;
	gboolean            is_fitting;

	GtkScalableImagePrivate* priv;
};

struct _GtkScalableImageClass
{
	GtkWidgetClass base;
};


GType          gtk_scalable_image_get_type           () G_GNUC_CONST;
GtkWidget*     gtk_scalable_image_new                ();
GtkWidget*     gtk_scalable_image_new_from_pixbuf    (GdkPixbuf*        pixbuf);
GdkPixbuf*     gtk_scalable_image_get_pixbuf         (GtkScalableImage* self);
void           gtk_scalable_image_set_pixbuf         (GtkScalableImage* self,
                                                      GdkPixbuf*        pixbuf);
double         gtk_scalable_image_get_scale          (GtkScalableImage* self);
void           gtk_scalable_image_set_scale          (GtkScalableImage* self,
                                                      double            scale);
void           gtk_scalable_image_set_scale_at_point (GtkScalableImage* self,
                                                      double            scale,
                                                      gint              viewport_x,
                                                      gint              viewport_y);
void           gtk_scalable_image_set_scale_to_fit   (GtkScalableImage* self);
void           gtk_scalable_image_translate          (GtkScalableImage* self,
                                                      gint              delta_x,
                                                      gint              delta_y);


G_END_DECLS

#endif
