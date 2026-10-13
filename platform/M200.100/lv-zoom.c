/** \file
 * Where Canon's magnified (x5 / x10) LiveView is inside the raw buffer, M200.
 *
 * In magnify the sensor reads a 1:1 window of 4288x1202 (with the optical
 * black), but Canon's view is not centred in it: the readout start only
 * moves in coarse steps (8 columns, 2 lines; two horizontal positions), and
 * the view (1200x800 at x5) moves freely inside it. Canon's imager keeps
 * all of this in its DataStore (IMAGER_DATASTORE_ADDR, consts.h):
 * - +0xD58 / +0xD5C: the movie mode's area, 5976x3976 (FHD / HD) or a
 *   centred 3840x2556 (4K);
 * - +0xD78 / +0xD7C: top left of the magnified view, in that area;
 * - +0x460: view size, height << 16 | width;
 * - +0x454: readout start (Imager_SetCmosSkipPointToDTS), x << 16 | y.
 * The mapping below matched Canon's LV image to the raw buffer within 2
 * pixels at 27 zoom positions in FHD and 4K. Read only.
 */
/*
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License
 * as published by the Free Software Foundation; either version 2
 * of the License, or (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the
 * Free Software Foundation, Inc.,
 * 51 Franklin Street, Fifth Floor,
 * Boston, MA  02110-1301, USA.
 */

#include <dryos.h>
#include <propvalues.h>
#include <consts.h>
#include <raw.h>

#ifdef CONFIG_RAW_LV_ZOOM_VIEW_POS

#define DS(off) (*(volatile uint32_t *)(IMAGER_DATASTORE_ADDR + (off)))

/* the FHD / HD area; 4K uses a smaller area centred in it */
#define FULL_AREA_W 5976
#define FULL_AREA_H 3976

int raw_lv_zoom_view_center(int *x, int *y)
{
    if (!lv || lv_dispsize == 1)
        return 0;

    int area_w = DS(0xD58);
    int area_h = DS(0xD5C);
    int view_x = DS(0xD78);
    int view_y = DS(0xD7C);
    int view_w = DS(0x460) & 0xFFFF;
    int view_h = DS(0x460) >> 16;
    int skip_x = DS(0x454) >> 16;
    int skip_y = DS(0x454) & 0xFFFF;

    if (area_w <= 0 || area_w > FULL_AREA_W || area_h <= 0 || area_h > FULL_AREA_H ||
        view_w <= 0 || view_h <= 0)
        return 0;

    *x = view_x + (FULL_AREA_W - area_w) / 2 + 288 - 8 * skip_x + view_w / 2;
    *y = view_y + (FULL_AREA_H - area_h) / 2 + 58 - 2 * skip_y + view_h / 2;

    /* outside the raw buffer? then this is not the mode we measured */
    if (*x < 0 || *x >= raw_info.width || *y < 0 || *y >= raw_info.height)
        return 0;

    return 1;
}

#endif
