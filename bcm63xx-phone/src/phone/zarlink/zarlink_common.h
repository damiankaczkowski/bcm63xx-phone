/*
 * Copyright (C) 2015
 * Gilles Mazoyer <mazoyer.gilles@omega.ovh>
 *
 * This is free software, licensed under the GNU General Public License v2.
 * See /LICENSE for more information.
 */

#ifndef __ZARLINK_COMMON_H__
#define __ZARLINK_COMMON_H__

#include <config.h>

#include <vp_api.h>

#include <bcm63xx_phone.h>
#include <phone.h>

typedef struct {
   const VpProfileDataType *dev_profile;
   const VpProfileDataType *default_AC_profile_NB;
   const VpProfileDataType *AC_profiles_NB[BCMPH_COUNTRY_MAX];
   const VpProfileDataType *default_AC_profile_WB;
   const VpProfileDataType *AC_profiles_WB[BCMPH_COUNTRY_MAX];
   const VpProfileDataType *default_DC_profile;
   const VpProfileDataType *DC_profiles[BCMPH_COUNTRY_MAX];
   const VpProfileDataType *default_ring_profile;
   const VpProfileDataType *ring_profiles[BCMPH_COUNTRY_MAX];
} zarlink_profiles_t;

typedef struct {
   const VpProfileDataType *profile;
   __u16 on_time; /* in msec */
   __u16 off_time; /* in msec */
} zarlink_tone_t;

typedef struct zarlink_device_parameters {
   VpDeviceType type;
   zarlink_profiles_t profiles;
   struct {
      VpLineStateType on_hook_idle;
      VpLineStateType on_hook_idle_polrev;
      VpLineStateType on_hook_ringing;
      VpLineStateType on_hook_ringing_polrev;
      VpLineStateType on_hook_talking;
      VpLineStateType on_hook_talking_polrev;
      VpLineStateType off_hook_idle;
      VpLineStateType off_hook_idle_polrev;
      VpLineStateType off_hook_talking;
      VpLineStateType off_hook_talking_polrev;
      VpLineStateType disconnect;
   } modes;
   struct {
      zarlink_tone_t waiting_dial;
      zarlink_tone_t invalid;
      zarlink_tone_t ringback;
      zarlink_tone_t busy;
      zarlink_tone_t disconnect;
   } tones;
} zarlink_device_parameters_t;

typedef struct zarlink_line_parameters {
   uint8 id;
   VpTermType type;
} zarlink_line_parameters_t;

typedef struct {
   phone_device_t vd;
   const struct vtbl_phone_dev_zarlink *vtbl;
   zarlink_device_id_t device_id;
   struct {
      void *obj;
      VpDevCtxType ctx;
   } dev;
   struct {
      void *obj;
      VpLineCtxType *ctx;
      struct {
         bcm_phone_line_tone_t tone;
         const VpProfileDataType *z_profile;
         VpDigitType z_tone_id;
         __u16 on_time;
         __u16 off_time;
         bool is_on;
         __u16 timer;
      } tone_cadencer;
      struct {
         bcm_phone_line_mode_t mode_on;
         bcm_phone_line_mode_t mode_off;
         VpLineStateType z_mode_on;
         VpLineStateType z_mode_off;
         __u16 on_time;
         __u16 off_time;
         bool is_on;
         __u16 timer;
      } mode_cadencer;
   } lines[VP_MAX_LINES_PER_DEVICE];
   /*
    The timer when reaching 0 signals that we update the line state with codec,
    mode and/or tone asked by ioctls.
    We use a timer because we don't check the change at each call of tick
    but at a slower rate.
    We use a common timer for each line.
    This is quite logical as we get all the line's state at once.
   */
   __u32 line_update_timer;
} phone_dev_zarlink_t;

typedef struct vtbl_phone_dev_zarlink {
   vtbl_phone_device_t vd;
} vtbl_phone_dev_zarlink_t;

extern void phone_dev_zarlink_init(phone_dev_zarlink_t *t,
   const vtbl_phone_dev_zarlink_t *vtbl,
   const phone_desc_device_t *dev_desc, __u8 tick_period,
   void *dev_obj, bcm_mpi_t *mpi);

extern void phone_dev_zarlink_deinit(phone_dev_zarlink_t *t);

static inline void phone_dev_zarlink_init_line(phone_dev_zarlink_t *t,
   size_t index, phone_line_t *vl, void *line_obj, VpLineCtxType *line_ctx)
{
   d_bcm_pr_debug("%s(index=%lu)\n", __func__, (unsigned long)(index));
   bcm_assert((index < ARRAY_SIZE(t->lines))
      && (NULL != vl) && (NULL != line_obj) && (NULL != line_ctx));
   phone_device_init_line(&(t->vd), index, vl);
   t->lines[index].obj = line_obj;
   t->lines[index].ctx = line_ctx;
}

static inline void phone_dev_zarlink_deinit_line(phone_dev_zarlink_t *t,
   size_t index)
{
   d_bcm_pr_debug("%s(index=%lu)\n", __func__, (unsigned long)(index));
   bcm_assert(index < ARRAY_SIZE(t->lines));
   phone_device_deinit_line(&(t->vd), index);
   t->lines[index].obj = NULL;
   t->lines[index].ctx = NULL;
}

static inline const zarlink_device_id_t *phone_dev_zarlink_get_device_id(const phone_dev_zarlink_t *t)
{
   d_bcm_pr_debug("%s()\n", __func__);
   return (&(t->device_id));
}

static inline void *phone_dev_zarlink_get_dev_obj(phone_dev_zarlink_t *t)
{
   d_bcm_pr_debug("%s()\n", __func__);
   return (t->dev.obj);
}

static inline VpDevCtxType *phone_dev_zarlink_get_dev_ctx(phone_dev_zarlink_t *t)
{
   d_bcm_pr_debug("%s()\n", __func__);
   return (&(t->dev.ctx));
}

static inline void *phone_dev_zarlink_get_line_obj(phone_dev_zarlink_t *t, size_t index)
{
   d_bcm_pr_debug("%s(index=%lu)\n", __func__, (unsigned long)(index));
   bcm_assert(index < ARRAY_SIZE(t->lines));
   return (t->lines[index].obj);
}

static inline VpLineCtxType *phone_dev_zarlink_get_line_ctx(phone_dev_zarlink_t *t, size_t index)
{
   d_bcm_pr_debug("%s(index=%lu)\n", __func__, (unsigned long)(index));
   bcm_assert(index < ARRAY_SIZE(t->lines));
   return (t->lines[index].ctx);
}

extern void phone_dev_zarlink_tick(phone_device_t *d);

extern void phone_dev_zarlink_update_line_state_asap(phone_device_t *d, size_t index_line);

#ifdef VP_CC_880_SERIES

typedef struct {
   phone_dev_zarlink_t vdz;
   bcm_mpi_t mpi;
   Vp880DeviceObjectType dev_obj;
} phone_dev_zarlink_ve880_t;

extern int phone_dev_zarlink_ve880_init(phone_dev_zarlink_ve880_t *t,
   const vtbl_phone_dev_zarlink_t *vtbl,
   const phone_desc_device_t *dev_desc, __u8 tick_period);

extern void phone_dev_zarlink_ve880_deinit(phone_dev_zarlink_ve880_t *t);

#ifdef BCMPH_DEBUG
typedef struct {
   __u8 cmd;
   __u8 data_len;
} zarlink_cmd_desc_t;

extern void phone_dev_zarlink_ve880_pr_regs(phone_dev_zarlink_ve880_t *t);
#endif // BCMPH_DEBUG

extern int phone_dev_zarlink_ve880_start(phone_device_t *t,
   bcmph_country_t country,
   const phone_line_params_t * const *line_params, size_t line_count);

extern void phone_dev_zarlink_ve880_stop(phone_device_t *d);

extern void phone_dev_zarlink_ve880_set_cadence(phone_dev_zarlink_t *d,
   size_t line, __u16 cadence_on, __u16 cadence_off);

#endif // VP_CC_880_SERIES

#endif // __ZARLINK_COMMON_H__
