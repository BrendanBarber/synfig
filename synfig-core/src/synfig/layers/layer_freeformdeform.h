/* === S Y N F I G ========================================================= */
/*!	\file layer_freeformdeform.h
**	\brief FreeFormDeform layer
**
**	\legal
**	......... ... 2026 Brendan Barber
**
**	This file is part of Synfig.
**
**	Synfig is free software: you can redistribute it and/or modify
**	it under the terms of the GNU General Public License as published by
**	the Free Software Foundation, either version 2 of the License, or
**	(at your option) any later version.
**
**	Synfig is distributed in the hope that it will be useful,
**	but WITHOUT ANY WARRANTY; without even the implied warranty of
**	MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
**	GNU General Public License for more details.
**
**	You should have received a copy of the GNU General Public License
**	along with Synfig.  If not, see <https://www.gnu.org/licenses/>.
**	\endlegal
*/
/* ========================================================================= */

/* === S T A R T =========================================================== */

#ifndef __SYNFIG_LAYER_FREEFORMDEFORMATION_H
#define __SYNFIG_LAYER_FREEFORMDEFORMATION_H

/* === H E A D E R S ======================================================= */

#include "layer_meshtransform.h"
#include <synfig/rendering/common/task/taskmesh.h>

/* === C L A S S E S & S T R U C T S ======================================= */

namespace synfig {
/*!	\class Layer_FreeFormDeform
**	\brief Class of the FreeFormDeform layer.
*/
class Layer_FreeFormDeform : public Layer_MeshTransform
{
	SYNFIG_LAYER_MODULE_EXT

private:
	//! Parameter: (List of Point)
	synfig::ValueBase param_control_points;

	//! Parameter: (Point)
	synfig::ValueBase param_point1;
	//! Parameter: (Point)
	synfig::ValueBase param_point2;

	//! Parameter: (Integer)
	synfig::ValueBase param_lattice_cols;
	//! Parameter: (Integer)
	synfig::ValueBase param_lattice_rows;

	//! Parameter: (Integer)
	synfig::ValueBase param_mesh_cols;
	//! Parameter: (Integer)
	synfig::ValueBase param_mesh_rows;

	void reinit_control_points();

public:
	typedef etl::handle<Layer_FreeFormDeform>       Handle;
	typedef etl::handle<const Layer_FreeFormDeform> ConstHandle;
	typedef etl::loose_handle<Layer_FreeFormDeform> LooseHandle;

	Layer_FreeFormDeform();
	virtual ~Layer_FreeFormDeform();

	virtual String    get_local_name()const;
	virtual bool      set_param(const String &param, const synfig::ValueBase &value);
	virtual ValueBase get_param(const String &param)const;
	virtual Vocab     get_param_vocab()const;

	void prepare_mesh();

protected:
    virtual rendering::Task::Handle build_composite_fork_task_vfunc(ContextParams context_params, rendering::Task::Handle sub_task) const;
	virtual rendering::Task::Handle build_rendering_task_vfunc(Context context) const;
}; // END of class Layer_FreeFormDeform

}; // END of namespace synfig

#endif