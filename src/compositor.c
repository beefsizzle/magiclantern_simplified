/** \file
 * VRAM Compositor interface
 */

/**
 * There's a hardware accelerated compositor running on Digic 6+ cameras.
 * It has couple of variants (pure Ximr, XCM) - see further comments.
 *
 * For more detailed explanation on how this was discovered, look at forums:
 * https://www.magiclantern.fm/forum/index.php?topic=26024
 */

/**
 * Terminology:
 * Zico core    - Xtensa core, running DryOS, used for graphics acceeleration,
 *                GUI rendering and compositing. Connected with GPU (Takumi
 *                GV550 IP core)
 * mzrm         - Marius - Zico remote messaging (?). RPC used to talk between
 *                main ARM core (named Marius on D6) and Zico.
 * Ximr         - Render MIXer (?). Low level compositor on D6 and up.
 * Ximr Context - Data structure representing Ximr configuration, sent via mzrm
 *                using XimrExe() to Zico for render.
 * Input layer  - Layer (described by MARV struct) to be included in mix.
 * XOC          - Ximr Output Chunk. Buffer where final image is rendered.
 * XCM          - Ximr Context Maker. "Userspace" tool to deal with Ximr setup.
 *                Exists on D8 and up, from EOS R (M50 uses pure Ximr)
 */

#include "dryos.h"
#include "bmp.h"
#include "compositor.h"
#include "propvalues.h"

#ifdef CONFIG_COMPOSITOR_DEDICATED_LAYER
/**
 * refreshVrmsSurface (renamed to VMIX_TransferRectangleToVram somewhere around 850D)
 * is used by Canon code to render Input layers into XOC.
 *
 * On pure XIMR cameras it uses XIMR functions to setup Ximr Context
 */
extern void      refreshVrmsSurface();
extern uint32_t  display_refresh_needed;

/**
 * Stores our layer ID. Defaults to Canon GUI layer, so in case
 * alocation failed - everything falls back to non-compositing behaviour.
 */
int _rgb_vram_layer_id = CANON_GUI_LAYER_ID;
struct MARV * pNewLayer;

/**
 * Not sure if sync_caches() call is needed. It was when I was drawing
 * over Canon buffers, but now when we have our own may be unnecessary.
 */
void _compositor_force_redraw()
{
    display_refresh_needed = 1;
    ml_refresh_display_needed = 1;
    sync_caches();
    //refreshVrmsSurface(); // not needed as we do XimrExe() in bmp.c
}

struct MARV *_compositor_create_layer(uint32_t bmp_w, uint32_t bmp_h)
{
    // buffer for MARV structure
    struct MARV* pNewLayer = malloc(sizeof(struct MARV));
    if(pNewLayer == NULL) return NULL;
    
    // buffer for layer data, +0x100 for alignment
    uint8_t* pBitmapData = malloc((BMP_VRAM_SIZE * 4) + 0x100);
    if(pBitmapData == NULL) {
        free(pNewLayer); // cleanup
        return NULL;
    }
    
    // Align to 0x100 and use uncacheable region.
    // This is expected by some Zico functions.
    pBitmapData = UNCACHEABLE((uint8_t*)((((uintptr_t)pBitmapData + 0x100) >> 8) << 8));

    // fill MARV data
    pNewLayer->signature    = 0x5652414D;  // MARV
    pNewLayer->bitmap_data  = pBitmapData;
    pNewLayer->opacity_data = 0x0;
    pNewLayer->flags        = XIMR_FLAGS_LAYER_RGBA; // see compositor.h
#ifdef CONFIG_DIGIC_X
    extern uint64_t MemifWindow_GetIBusAddress(uint8_t* buf);
    pNewLayer->memif_1      = 0xFFFFFFFF;   // no idea
    pNewLayer->ibus_addr    = MemifWindow_GetIBusAddress(CACHEABLE(pBitmapData));
#endif
    pNewLayer->width        = (uint32_t)bmp_w;
    pNewLayer->height       = (uint32_t)bmp_h;
    // Probably pmem is not needed. No issues observed so far.
    pNewLayer->pmem         = 0x0;

    DryosDebugMsg(0, 15, "MARV 0x%08x, bitmap_data 0x%08x", pNewLayer, pBitmapData);

    return pNewLayer;
}

#ifdef CONFIG_COMPOSITOR_XCM
/**
 * Implementation specific to models with Ximr Context Maker (XCM)
 */

#if !defined(CONFIG_COMPOSITOR_XCM_V1) && !defined(CONFIG_COMPOSITOR_XCM_V2)
#error "CONFIG_COMPOSITOR_XCM enabled, but no XCM type was defined!"
#endif

#ifdef CONFIG_DIGIC_X
#error XCM is not yet implemented for DIGIC_X, see compositor.c for details!
/*
 * Digic X uses the new "IBus" stuff for some kind of memory banking
 * We don't know how to use it yet, and Ximr now requires buffers to be
 * "on specific ibus". Otherwise camera aserts.
 *
 * I left in my best guess on Digic X implementation, but it requires malloc
 * replacment that will satisfy Ximr IBus requirements.
 */
#endif

#ifdef CONFIG_COMPOSITOR_XCM_V1
/*
 * Structures specific to EOS R / PowerShot SX740
 * XCM_SetSourceSurface(), XCM_SetSourceArea(), XOC_SetLayerEnable() calls are
 * done on each redraw (in refreshVrmsSurface() ) based on structures below.
 *
 * VMIX_Layers        List of *MARV for all existing layers
 * VMIX_LayersEnable  Controls if layer with given ID is displayed or not.
 */
extern struct MARV *VMIX_Layers[XIMR_MAX_LAYERS];
#ifdef CONFIG_R
extern uint32_t VMIX_LayersEnable[XIMR_MAX_LAYERS]; // R only, DNE on SX740
#endif // CONFIG_R
#endif // CONFIG_COMPOSITOR_XCM_V1

#ifdef CONFIG_COMPOSITOR_XCM_V2
/*
 * Pure XCM API is used
 */
extern uint32_t XCM_SetSourceSurface(void * pXCM, uint layer, struct MARV * pMARV);
extern uint32_t XCM_SetSourceArea(void * pXCM, uint layer, uint16_t srcX, uint16_t srcY, uint16_t srcW, uint16_t srcH);
extern uint32_t XOC_SetLayerEnable(int p1, int p2, uint layer, int p4);
#endif // CONFIG_COMPOSITOR_XCM_V2

/*
 * CONFIG_COMPOSITOR_XCM_V2 models use a single array of structures, and won't
 * override settings of the layers that we've created.
 * Left for reference only:
 *
 * struct RENDERER_LayerMedatata
 * {
 *   struct MARV* pMARV;  // pointer to layer MARV
 *    uint32_t w;   // layer width
 *    uint32_t h;   // layer height
 * };
 * extern struct RENDERER_LayerMedatata RENDERER_LayersMetadata[XIMR_MAX_LAYERS];
 *
 * ======
 *
 * On models since 850D(?), XCM_SetColorMatrixType() was added to XCM API.
 * extern uint32_t XCM_SetColorMatrixType(void * pXCM, uint layer, uint matrixType);
 * As tested on 850D: matrixType 0, 1, 2
 * 0: RGB->BT709FULL
 * 1: RGB->BT709VIDEO
 * 2: RGB->BT709FULL (?!) WINSYS_getMixColorMatrixIndex supports only 0-1
 *
 * Since XCM defaults to 0 on a new layer, we ommit this call.
 */

#ifdef CONFIG_COMPOSITOR_LAYER_LV_ONLY
/*
 * M200: with ML's layer present, switching between scene / art modes (LV
 * preview rendered with effects) and P/Tv/Av/M left the LV image frozen
 * until power cycle. The layer is only attached while LV runs, and its
 * bitmap only exists while attached, so Canon has that memory back when it
 * rebuilds the display for another mode. Outside LV, ML draws on Canon's
 * GUI layer.
 */
static int lv_layer_id = -1;
static int lv_layer_attached = 0;
static struct MARV *canon_gui_marv = NULL;
static void *lv_layer_raw = NULL;
static int lv_layer_free_countdown = 0;
static int lv_layer_alloc_failed = 0;   /* retry after the next LV start */
extern struct semaphore *winsys_sem;

static int lv_layer_alloc()
{
    if (lv_layer_raw)
        return 1;
    lv_layer_raw = malloc(BMP_VRAM_SIZE * 4 + 0x100);
    if (!lv_layer_raw)
        return 0;
    /* aligned to 0x100 and uncacheable, as _compositor_create_layer() */
    pNewLayer->bitmap_data = UNCACHEABLE((uint8_t *)((((uintptr_t)lv_layer_raw + 0x100) >> 8) << 8));
    return 1;
}

static void lv_layer_free()
{
    if (!lv_layer_raw)
        return;
    pNewLayer->bitmap_data = NULL;
    free(lv_layer_raw);
    lv_layer_raw = NULL;
}

static void lv_layer_attach()
{
    lv_layer_free_countdown = 0;
    if (!lv_layer_alloc())
    {
        lv_layer_alloc_failed = 1;
        return;
    }

    /* drop what we drew on Canon's layer while detached */
    rgba_erase_own_pixels();

    take_semaphore(winsys_sem, 0);
    XCM_SetSourceSurface(_pXCM, lv_layer_id, pNewLayer);
    XCM_SetSourceArea(_pXCM, lv_layer_id, -BMP_W_MINUS, -BMP_H_MINUS, BMP_W_PLUS + BMP_W_MINUS, BMP_H_PLUS + BMP_H_MINUS);
    XOC_SetLayerEnable(0, 0, lv_layer_id, 1);
    give_semaphore(winsys_sem);

    bzero32(pNewLayer->bitmap_data, BMP_VRAM_SIZE * 4);
    rgb_vram_info = pNewLayer;
    _rgb_vram_layer_id = lv_layer_id;
    lv_layer_attached = 1;
    rgba_shadow_invalidate();
    _compositor_force_redraw();
}

static void lv_layer_detach()
{
    take_semaphore(winsys_sem, 0);
    XCM_SetSourceSurface(_pXCM, lv_layer_id, NULL);
    give_semaphore(winsys_sem);

    rgb_vram_info = canon_gui_marv;
    _rgb_vram_layer_id = CANON_GUI_LAYER_ID;
    lv_layer_attached = 0;
    rgba_shadow_invalidate();
    _compositor_force_redraw();

    /* free the bitmap once Canon has rebuilt its context without it */
    lv_layer_free_countdown = 10;   /* x 50 ms */
}

/* called from the redraw task (bmp.c), every 50 ms */
void compositor_lv_layer_step()
{
    static int lv_since = 0;

    if (lv_layer_id < 0 || !canon_gui_marv)
        return;

    if (lv_layer_free_countdown && --lv_layer_free_countdown == 0)
        lv_layer_free();

    if (!lv)
    {
        lv_since = 0;
        lv_layer_alloc_failed = 0;
        if (lv_layer_attached)
            lv_layer_detach();
        return;
    }

    int now = get_ms_clock();
    if (!lv_since)
        lv_since = now;
    if (!lv_layer_attached && !lv_layer_alloc_failed && now - lv_since >= 1000)
        lv_layer_attach();
}
#endif

/*
 * Create a new VRAM (MARV) structure, alloc buffer for VRAM.
 * Call compositor to enable newly created layer.
 */
int compositor_layer_setup()
{
    // So far it seems that default GUI layer is always 0
    int newLayerID = 0;

    // Find first available, non used layer slot
    while( newLayerID < XIMR_MAX_LAYERS 
      && XCM_GetSourceSurface(_pXCM, newLayerID) != NULL)
    {
        newLayerID++;
    }

    DryosDebugMsg(0, 15, "Found %d layers", newLayerID);
    if(newLayerID >= XIMR_MAX_LAYERS)
    {
        DryosDebugMsg(0, 15, "Too many layers: %d/%d, aborting!",
                newLayerID, XIMR_MAX_LAYERS);
        return 1;
    }

    // compute our layer size
    uint32_t bmp_w = BMP_W_PLUS - BMP_W_MINUS;
    uint32_t bmp_h = BMP_H_PLUS - BMP_H_MINUS;

    // create layer
#ifdef CONFIG_COMPOSITOR_LAYER_LV_ONLY
    /* MARV only; the bitmap is allocated when the layer gets attached */
    pNewLayer = malloc(sizeof(struct MARV));
    if (pNewLayer)
    {
        pNewLayer->signature    = 0x5652414D;  // MARV
        pNewLayer->bitmap_data  = NULL;
        pNewLayer->opacity_data = 0x0;
        pNewLayer->flags        = XIMR_FLAGS_LAYER_RGBA;
        pNewLayer->width        = bmp_w;
        pNewLayer->height       = bmp_h;
        pNewLayer->pmem         = 0x0;
    }
#else
    pNewLayer = _compositor_create_layer(bmp_w, bmp_h);
#endif
    if(pNewLayer == NULL){
        DryosDebugMsg(0, 15, "Failed to create a new layer. Falling back to _rgb_vram_info");
        return 1;
    }
#ifdef CONFIG_COMPOSITOR_LAYER_LV_ONLY
    /* attached / detached by compositor_lv_layer_step() */
    lv_layer_id = newLayerID;
    canon_gui_marv = rgb_vram_info;     /* Canon GUI layer, from rgb_vram_preinit() */
    return 0;
#endif
    /*
     * The code below seems to be R specific. RP/R6 seems to use single struct
     * for that.
     */

#ifdef CONFIG_COMPOSITOR_XCM_V1
    VMIX_Layers[newLayerID] = pNewLayer;
#ifdef CONFIG_R
    VMIX_LayersEnable[newLayerID] = 1;
#endif // CONFIG_R
#endif // CONFIG_COMPOSITOR_XCM_V1

#ifdef CONFIG_COMPOSITOR_XCM_V2
    XCM_SetSourceSurface(_pXCM, newLayerID, pNewLayer);
    XCM_SetSourceArea(_pXCM, newLayerID, -BMP_W_MINUS, -BMP_H_MINUS, BMP_W_PLUS + BMP_W_MINUS, BMP_H_PLUS + BMP_H_MINUS);
    XOC_SetLayerEnable(0, 0, newLayerID, 1); // seems layer is enabled by default
#endif // CONFIG_COMPOSITOR_XCM_V2

    // save rgb_vram_info as last step, in case something above fails.
    rgb_vram_info   = pNewLayer;
    _rgb_vram_layer_id = newLayerID;

    // erase buffer and force redraw
    rgba_buffer_clear();
    return 0;
}

/*
// TODO: Write implementationf for pure Ximr, if we ever want it.
#elif defined(CONFIG_COMPOSITOR_XIMR)
*/
#endif // CONFIG_COMPOSITOR_XCM

#endif // CONFIG_COMPOSITOR_DEDICATED_LAYER
