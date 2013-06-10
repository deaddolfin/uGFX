/*
 * This file is subject to the terms of the GFX License, v1.0. If a copy of
 * the license was not distributed with this file, you can obtain one at:
 *
 *              http://chibios-gfx.com/license.html
 */

/**
 * @file    include/gwin/slider.h
 * @brief   GWIN Graphic window subsystem header file.
 *
 * @defgroup Slider Slider
 * @ingroup GWIN
 *
 * @details		Create sliders with different styles
 *
 * @pre			GFX_USE_GWIN must be set to TRUE in your gfxconf.h
 * @pre			GWIN_NEED_SLIDER must be set to TRUE in your gfxconf.h
 * @{
 */

#ifndef _GWIN_SLIDER_H
#define _GWIN_SLIDER_H

/* This file is included within "gwin/gwidget.h" */

#define GEVENT_GWIN_SLIDER		(GEVENT_GWIN_FIRST+1)

typedef struct GEventGWinSlider_t {
	GEventType		type;				// The type of this event (GEVENT_GWIN_BUTTON)
	GHandle			slider;				// The slider that is returning results
	int				position;
} GEventGWinSlider;

// There are currently no GEventGWinSlider listening flags - use 0

typedef struct GSliderColors {
	color_t				color_edge;
	color_t				color_thumb;
	color_t				color_active;
	color_t				color_inactive;
	color_t				color_txt;
} GSliderColors;

// A slider window
typedef struct GSliderObject_t {
	GWidgetObject		w;
	uint16_t			t_dn;
	uint16_t			t_up;
	uint16_t			dial;
	coord_t				dpos;
	int					min;
	int					max;
	int					pos;
	GSliderColors		c;
} GSliderObject;

#ifdef __cplusplus
extern "C" {
#endif

/**
 * @brief   Create a slider window.
 * @return  NULL if there is no resultant drawing area, otherwise a window handle.
 *
 * @param[in] gb		The GSliderObject structure to initialise. If this is NULL the structure is dynamically allocated.
 * @param[in] x,y		The screen co-ordinates for the top left corner of the window
 * @param[in] width		The width of the window
 * @param[in] height	The height of the window
 *
 * @note				The drawing color and the background color get set to the current defaults. If you haven't called
 * 						@p gwinSetDefaultColor() or @p gwinSetDefaultBgColor() then these are White and Black respectively.
 * @note				The font gets set to the current default font. If you haven't called @p gwinSetDefaultFont() then there
 * 						is no default font and text drawing operations will no nothing.
 * @note				The dimensions and position may be changed to fit on the real screen.
 * @note				A slider remembers its normal drawing state. If there is a window manager then it is automatically
 * 						redrawn if the window is moved or its visibility state is changed.
 * @note				The slider is initially marked as invisible so that more properties can be set before display.
 * 						Call @p gwinSetVisible() to display it when ready.
 * @note				The initial slider range is from 0 to 100 with an initial position of 0.
 * @note				A slider supports mouse, toggle and dial input.
 * @note				When assigning a toggle, only one toggle is supported per role. If you try to assign more than
 * 						one toggle to a role it will forget the previous toggle. Two roles are supported:
 * 						Role 0 = toggle for down, Role 1 = toggle for up.
 * @note				When assigning a dial, only one dial is supported. If you try to assign more than one dial
 * 						it will forget the previous dial. Only dial role 0 is supported.
 *
 * @api
 */	
GHandle gwinCreateSlider(GSliderObject *gb, coord_t x, coord_t y, coord_t width, coord_t height);

/**
 * @brief   Set the slider range.
 *
 * @param[in] gh		The window handle (must be a slider window)
 * @param[in] min		The minimum value
 * @param[in] max		The maximum value
 * @note				Sets the position to the minimum value.
 * @note				The slider is not automatically drawn. Call gwinSliderDraw() after changing the range.
 *
 * @api
 */
void gwinSetSliderRange(GHandle gh, int min, int max);

/**
 * @brief   Set the slider position.
 *
 * @param[in] gh		The window handle (must be a slider window)
 * @param[in] pos		The new position
 * @note				If the new position is outside the slider range then the position
 * 						is set to the closest end of the range.
 * @note				The slider is not automatically drawn. Call gwinSliderDraw() after changing the position.
 *
 * @api
 */
void gwinSetSliderPosition(GHandle gh, int pos);

/**
 * @brief   Set the style of a slider.
 * @details	The slider style is defined by its colours.
 *
 * @param[in] gh		The window handle (must be a slider window)
 * @param[in] pStyle	The styling for the slider.
 *
 * @note				The slider is not automatically redrawn. Call gwinSliderDraw() after changing the slider style
 * @note				The slider style is copied into the internal slider structure - there is no need to
 * 						maintain a static style structure.
 *
 * @api
 */
void gwinSetSliderColors(GHandle gh, const GSliderColors *pStyle);

/**
 * @brief   Get the current slider position.
 * @return	The slider position
 *
 * @param[in] gh		The window handle (must be a slider window)
 *
 * @note	The use of a listener to get the slider position is recommended if you
 * 			want continuous updates on the slider position.
 *
 * @api
 */
#define gwinGetSliderPosition(gh)		(((GSliderObject *)(gh))->pos)

/**
 * @brief	Some custom slider drawing routines
 * @details	These function may be passed to @p gwinSetCustomDraw() to get different slider drawing styles
 *
 * @param[in] gw			The widget (which must be a slider)
 * @param[in] param			A parameter passed in from the user
 *
 * @note				In your custom slider drawing function you may optionally call this
 * 						standard functions and then draw your extra details on top.
 * @note				The standard functions below ignore the param parameter except for @p gwinSliderDraw_Image().
 * @note				The image custom draw function  @p gwinSliderDraw_Image() uses param to pass in the gdispImage pointer.
 * 						The image must be already opened before calling  @p gwinSetCustomDraw(). The image is tiled to fill
 * 						the active area of the slider. The normal colors apply to the border and inactive area and the dividing line
 * 						between the active and inactive areas.
 * 						No checking is done to compare the dimensions of the slider to the size of the image.
 * 						Note text is drawn on top of the image.
 * @note				These custom drawing routines don't have to worry about setting clipping as the framework
 * 						sets clipping to the object window prior to calling these routines.
 *
 * @api
 * @{
 */
void gwinSliderDraw_Std(GWidgetObject *gw, void *param);
void gwinSliderDraw_Image(GWidgetObject *gw, void *param);
/* @} */

#ifdef __cplusplus
}
#endif

#endif /* _GWIN_SLIDER_H */
/** @} */

