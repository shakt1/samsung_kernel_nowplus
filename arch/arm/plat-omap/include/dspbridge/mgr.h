/*
 * mgr.h
 *
 * DSP-BIOS Bridge driver support functions for TI OMAP processors.
 *
 * This is the Class driver RM module interface.
 *
 * Copyright (C) 2005-2006 Texas Instruments, Inc.
 *
 * This package is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2 as
 * published by the Free Software Foundation.
 *
 * THIS PACKAGE IS PROVIDED ``AS IS'' AND WITHOUT ANY EXPRESS OR
 * IMPLIED WARRANTIES, INCLUDING, WITHOUT LIMITATION, THE IMPLIED
 * WARRANTIES OF MERCHANTIBILITY AND FITNESS FOR A PARTICULAR PURPOSE.
 */

#ifndef MGR_
#define MGR_

#include <dspbridge/mgrpriv.h>

#define MAX_EVENTS 32

/*
 *  ======== mgr_wait_for_bridge_events ========
 *  Purpose:
 *      Block on any Bridge event(s)
 *  Parameters:
 *      anotifications  : array of pointers to notification objects.
 *      count          : number of elements in above array
 *      pu_index         : index of signaled event object
 *      utimeout        : timeout interval in milliseocnds
 *  Returns:
 *      DSP_SOK         : Success.
 *      -ETIME    : Wait timed out. *pu_index is undetermined.
 *  Details:
 */

dsp_status mgr_wait_for_bridge_events(struct dsp_notification
				      **anotifications,
				      u32 count, OUT u32 *pu_index,
				      u32 utimeout);

/*
 *  ======== mgr_create ========
 *  Purpose:
 *      Creates the Manager Object. This is done during the driver loading.
 *      There is only one Manager Object in the DSP/BIOS Bridge.
 *  Parameters:
 *      phMgrObject:    Location to store created MGR Object handle.
 *      dev_node_obj:       Device object as known to Windows system.
 *  Returns:
 *      DSP_SOK:        Success
 *      -ENOMEM:    Failed to Create the Object
 *      -EPERM:      General Failure
 *  Requires:
 *      MGR Initialized (refs > 0 )
 *      phMgrObject != NULL.
 *  Ensures:
 *      DSP_SOK:        *phMgrObject is a valid MGR interface to the device.
 *                      MGR Object stores the DCD Manager Handle.
 *                      MGR Object stored in the Regsitry.
 *      !DSP_SOK:       MGR Object not created
 *  Details:
 *      DCD Dll is loaded and MGR Object stores the handle of the DLL.
 */
extern dsp_status mgr_create(OUT struct mgr_object **hmgr_obj,
			     struct cfg_devnode *dev_node_obj);

/*
 *  ======== mgr_destroy ========
 *  Purpose:
 *      Destroys the MGR object. Called upon driver unloading.
 *  Parameters:
 *      hmgr_obj:     Handle to Manager object .
 *  Returns:
 *      DSP_SOK:        Success.
 *                      DCD Manager freed; MGR Object destroyed;
 *                      MGR Object deleted from the Registry.
 *      -EPERM:      Failed to destroy MGR Object
 *  Requires:
 *      MGR Initialized (refs > 0 )
 *      hmgr_obj is a valid MGR handle .
 *  Ensures:
 *      DSP_SOK:        MGR Object destroyed and hmgr_obj is Invalid MGR
 *                      Handle.
 */
extern dsp_status mgr_destroy(struct mgr_object *hmgr_obj);

/*
 *  ======== mgr_enum_node_info ========
 *  Purpose:
 *      Enumerate and get configuration information about nodes configured
 *      in the node database.
 *  Parameters:
 *      node_id:              The node index (base 0).
 *      pndb_props:          Ptr to the dsp_ndbprops structure for output.
 *      undb_props_size:      Size of the dsp_ndbprops structure.
 *      pu_num_nodes:         Location where the number of nodes configured
 *                          in the database will be returned.
 *  Returns:
 *      DSP_SOK:            Success.
 *      -EINVAL:    Parameter node_id is > than the number of nodes.
 *                          configutred in the system
 *      DSP_ECHANGEDURINGENUM:  During Enumeration there has been a change in
 *                              the number of nodes configured or in the
 *                              the properties of the enumerated nodes.
 *      -EPERM:          Failed to querry the Node Data Base
 *  Requires:
 *      pNDBPROPS is not null
 *      undb_props_size >= sizeof(dsp_ndbprops)
 *      pu_num_nodes is not null
 *      MGR Initialized (refs > 0 )
 *  Ensures:
 *      SUCCESS on successful retreival of data and *pu_num_nodes > 0 OR
 *      DSP_FAILED  && *pu_num_nodes == 0.
 *  Details:
 */
extern dsp_status mgr_enum_node_info(u32 node_id,
				     OUT struct dsp_ndbprops *pndb_props,
				     u32 undb_props_size,
				     OUT u32 *pu_num_nodes);

/*
 *  ======== mgr_enum_processor_info ========
 *  Purpose:
 *      Enumerate and get configuration information about available DSP
 *      processors
 *  Parameters:
 *      processor_id:         The processor index (zero-based).
 *      processor_info:     Ptr to the dsp_processorinfo structure .
 *      processor_info_size: Size of dsp_processorinfo structure.
 *      pu_num_procs:         Location where the number of DSPs configured
 *                          in the database will be returned
 *  Returns:
 *      DSP_SOK:            Success.
 *      -EINVAL:    Parameter processor_id is > than the number of
 *                          DSP Processors in the system.
 *      -EPERM:          Failed to querry the Node Data Base
 *  Requires:
 *      processor_info is not null
 *      pu_num_procs is not null
 *      processor_info_size >= sizeof(dsp_processorinfo)
 *      MGR Initialized (refs > 0 )
 *  Ensures:
 *      SUCCESS on successful retreival of data and *pu_num_procs > 0 OR
 *      DSP_FAILED && *pu_num_procs == 0.
 *  Details:
 */
extern dsp_status mgr_enum_processor_info(u32 processor_id,
					  OUT struct dsp_processorinfo
					  *processor_info,
					  u32 processor_info_size,
					  OUT u8 *pu_num_procs);
/*
 *  ======== mgr_exit ========
 *  Purpose:
 *      Decrement reference count, and free resources when reference count is
 *      0.
 *  Parameters:
 *  Returns:
 *  Requires:
 *      MGR is initialized.
 *  Ensures:
 *      When reference count == 0, MGR's private resources are freed.
 */
extern void mgr_exit(void);

/*
 *  ======== mgr_get_dcd_handle ========
 *  Purpose:
 *      Retrieves the MGR handle. Accessor Function
 *  Parameters:
 *      hMGRHandle:     Handle to the Manager Object
 *      phDCDHandle:    Ptr to receive the DCD Handle.
 *  Returns:
 *      DSP_SOK:        Sucess
 *      -EPERM:      Failure to get the Handle
 *  Requires:
 *      MGR is initialized.
 *      phDCDHandle != NULL
 *  Ensures:
 *      DSP_SOK and *phDCDHandle != NULL ||
 *      -EPERM and *phDCDHandle == NULL
 */
extern dsp_status mgr_get_dcd_handle(IN struct mgr_object
				     *hMGRHandle, OUT u32 *phDCDHandle);

/*
 *  ======== mgr_init ========
 *  Purpose:
 *      Initialize MGR's private state, keeping a reference count on each
 *      call. Intializes the DCD.
 *  Parameters:
 *  Returns:
 *      TRUE if initialized; FALSE if error occured.
 *  Requires:
 *  Ensures:
 *      TRUE: A requirement for the other public MGR functions.
 */
extern bool mgr_init(void);

#endif /* MGR_ */
