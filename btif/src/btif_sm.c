/******************************************************************************
 *
 *  Copyright (C) 2009-2012 Broadcom Corporation
 *
 *  This program is the proprietary software of Broadcom Corporation and/or its
 *  licensors, and may only be used, duplicated, modified or distributed
 *  pursuant to the terms and conditions of a separate, written license
 *  agreement executed between you and Broadcom (an "Authorized License").
 *  Except as set forth in an Authorized License, Broadcom grants no license
 *  (express or implied), right to use, or waiver of any kind with respect to
 *  the Software, and Broadcom expressly reserves all rights in and to the
 *  Software and all intellectual property rights therein.
 *  IF YOU HAVE NO AUTHORIZED LICENSE, THEN YOU HAVE NO RIGHT TO USE THIS
 *  SOFTWARE IN ANY WAY, AND SHOULD IMMEDIATELY NOTIFY BROADCOM AND DISCONTINUE
 *  ALL USE OF THE SOFTWARE.
 *
 *  Except as expressly set forth in the Authorized License,
 *
 *  1.     This program, including its structure, sequence and organization,
 *         constitutes the valuable trade secrets of Broadcom, and you shall
 *         use all reasonable efforts to protect the confidentiality thereof,
 *         and to use this information only in connection with your use of
 *         Broadcom integrated circuit products.
 *
 *  2.     TO THE MAXIMUM EXTENT PERMITTED BY LAW, THE SOFTWARE IS PROVIDED
 *         "AS IS" AND WITH ALL FAULTS AND BROADCOM MAKES NO PROMISES,
 *         REPRESENTATIONS OR WARRANTIES, EITHER EXPRESS, IMPLIED, STATUTORY,
 *         OR OTHERWISE, WITH RESPECT TO THE SOFTWARE.  BROADCOM SPECIFICALLY
 *         DISCLAIMS ANY AND ALL IMPLIED WARRANTIES OF TITLE, MERCHANTABILITY,
 *         NONINFRINGEMENT, FITNESS FOR A PARTICULAR PURPOSE, LACK OF VIRUSES,
 *         ACCURACY OR COMPLETENESS, QUIET ENJOYMENT, QUIET POSSESSION OR
 *         CORRESPONDENCE TO DESCRIPTION. YOU ASSUME THE ENTIRE RISK ARISING
 *         OUT OF USE OR PERFORMANCE OF THE SOFTWARE.
 *
 *  3.     TO THE MAXIMUM EXTENT PERMITTED BY LAW, IN NO EVENT SHALL BROADCOM
 *         OR ITS LICENSORS BE LIABLE FOR
 *         (i)   CONSEQUENTIAL, INCIDENTAL, SPECIAL, INDIRECT, OR EXEMPLARY
 *               DAMAGES WHATSOEVER ARISING OUT OF OR IN ANY WAY RELATING TO
 *               YOUR USE OF OR INABILITY TO USE THE SOFTWARE EVEN IF BROADCOM
 *               HAS BEEN ADVISED OF THE POSSIBILITY OF SUCH DAMAGES; OR
 *         (ii)  ANY AMOUNT IN EXCESS OF THE AMOUNT ACTUALLY PAID FOR THE
 *               SOFTWARE ITSELF OR U.S. $1, WHICHEVER IS GREATER. THESE
 *               LIMITATIONS SHALL APPLY NOTWITHSTANDING ANY FAILURE OF
 *               ESSENTIAL PURPOSE OF ANY LIMITED REMEDY.
 *
 *****************************************************************************/


/*****************************************************************************
 *
 *  Filename:      btif_sm.c
 *
 *  Description:   Generic BTIF state machine API
 * 
 *****************************************************************************/
#include <hardware/bluetooth.h>

#define LOG_TAG "BTIF_SM"
#include "btif_common.h"
#include "btif_sm.h"
#include "gki.h"

/*****************************************************************************
**  Constants & Macros
******************************************************************************/


/*****************************************************************************
**  Local type definitions
******************************************************************************/
typedef struct {
  btif_sm_state_t state;
  btif_sm_handler_t *p_handlers;
} btif_sm_cb_t;

/*****************************************************************************
**  Static variables
******************************************************************************/

/*****************************************************************************
**  Static functions
******************************************************************************/

/*****************************************************************************
**  Externs
******************************************************************************/

/*****************************************************************************
**  Functions
******************************************************************************/

/*****************************************************************************
**
** Function     btif_sm_init
**
** Description  Initializes the state machine with the state handlers
**              The caller should ensure that the table and the corresponding
**              states match. The location that 'p_handlers' points to shall
**              be available until the btif_sm_shutdown API is invoked.
**
** Returns      Returns a pointer to the initialized state machine handle.
**
******************************************************************************/
btif_sm_handle_t btif_sm_init(const btif_sm_handler_t *p_handlers, btif_sm_state_t initial_state)
{
    btif_sm_cb_t *p_cb;

    if (p_handlers == NULL)
    {
        BTIF_TRACE_ERROR1("%s : p_handlers is NULL", __FUNCTION__);
        return NULL;
    }

    p_cb = (btif_sm_cb_t*) GKI_os_malloc(sizeof(btif_sm_cb_t));
    p_cb->state = initial_state;
    p_cb->p_handlers = (btif_sm_handler_t*)p_handlers;

    /* Send BTIF_SM_ENTER_EVT to the initial state */
    p_cb->p_handlers[initial_state](BTIF_SM_ENTER_EVT, NULL);
    
    return (btif_sm_handle_t)p_cb;
}

/*****************************************************************************
**
** Function     btif_sm_shutdown
**
** Description  Tears down the state machine
**
** Returns      None
**
******************************************************************************/
void btif_sm_shutdown(btif_sm_handle_t handle)
{
    btif_sm_cb_t *p_cb = (btif_sm_cb_t*)handle;

    if (p_cb == NULL)
    {
        BTIF_TRACE_ERROR1("%s : Invalid handle", __FUNCTION__);
        return;
    }
    GKI_os_free((void*)p_cb);
}

/*****************************************************************************
**
** Function     btif_sm_get_state
**
** Description  Fetches the current state of the state machine
**
** Returns      Current state
**
******************************************************************************/
btif_sm_state_t btif_sm_get_state(btif_sm_handle_t handle)
{
    btif_sm_cb_t *p_cb = (btif_sm_cb_t*)handle;

    if (p_cb == NULL)
    {
        BTIF_TRACE_ERROR1("%s : Invalid handle", __FUNCTION__);
        return 0;
    }

    return p_cb->state;
}

/*****************************************************************************
**
** Function     btif_sm_dispatch
**
** Description  Dispatches the 'event' along with 'data' to the current state handler
**
** Returns      Returns BT_STATUS_OK on success, BT_STATUS_FAIL otherwise
**
******************************************************************************/
bt_status_t btif_sm_dispatch(btif_sm_handle_t handle, btif_sm_event_t event,
                                void *data)
{
    btif_sm_cb_t *p_cb = (btif_sm_cb_t*)handle;

    if (p_cb == NULL)
    {
        BTIF_TRACE_ERROR1("%s : Invalid handle", __FUNCTION__);
        return BT_STATUS_FAIL;
    }

    p_cb->p_handlers[p_cb->state](event, data);

    return BT_STATUS_SUCCESS;
}

/*****************************************************************************
**
** Function     btif_sm_change_state
**
** Description  Make a transition to the new 'state'. The 'BTIF_SM_EXIT_EVT'
**              shall be invoked before exiting the current state. The
**              'BTIF_SM_ENTER_EVT' shall be invoked before entering the new state
**
** Returns      Returns BT_STATUS_SUCCESS on success, BT_STATUS_FAIL otherwise
**
******************************************************************************/
bt_status_t btif_sm_change_state(btif_sm_handle_t handle, btif_sm_state_t state)
{
    btif_sm_cb_t *p_cb = (btif_sm_cb_t*)handle;

    if (p_cb == NULL)
    {
        BTIF_TRACE_ERROR1("%s : Invalid handle", __FUNCTION__);
        return BT_STATUS_FAIL;
    }

    /* Send exit event to the current state */
    p_cb->p_handlers[p_cb->state](BTIF_SM_EXIT_EVT, NULL);

    /* Change to the new state */
    p_cb->state = state;

    /* Send enter event to the new state */
    p_cb->p_handlers[p_cb->state](BTIF_SM_ENTER_EVT, NULL);

    return BT_STATUS_SUCCESS;
}