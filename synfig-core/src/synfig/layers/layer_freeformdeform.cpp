/* === S Y N F I G ========================================================= */
/*!	\file layer_freeformdeform.cpp
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

#ifdef USING_PCH
#	include "pch.h"
#else
#ifdef HAVE_CONFIG_H
#	include <config.h>
#endif

#include "layer_freeformdeform.h"

#include <synfig/localization.h>
#include <synfig/context.h>
#include <synfig/paramdesc.h>
#include <synfig/string.h>
#include <synfig/value.h>

#include <synfig/rendering/common/task/taskblend.h>
#include <synfig/rendering/common/task/tasklayer.h>
#include <synfig/rendering/common/task/taskmesh.h>

#include <vector>
#include <algorithm>

#endif

/* === U S I N G =========================================================== */

using namespace synfig;

/* === M A C R O S ========================================================= */

/* === C L A S S E S ======================================================= */

/* === G L O B A L S ======================================================= */

SYNFIG_LAYER_INIT(Layer_FreeFormDeform);
SYNFIG_LAYER_SET_NAME(Layer_FreeFormDeform,"freeform_deform");
SYNFIG_LAYER_SET_LOCAL_NAME(Layer_FreeFormDeform,N_("Free Form Deform"));
SYNFIG_LAYER_SET_CATEGORY(Layer_FreeFormDeform,N_("Distortions"));
SYNFIG_LAYER_SET_VERSION(Layer_FreeFormDeform,"0.1");

/* === M E T H O D S ======================================================= */

static Real bernstein(int n, int k, Real t)
{
	// https://en.wikipedia.org/wiki/Bernstein_polynomial
	
	// Calculate binomial coefficient (n choose k)
	Real binom = 1.0;
	for (int i = 0; i < k; ++i) 
        binom *= (Real)(n - i) / (Real)(i + 1);

	// Calculat t^k
	Real tk = 1.0;
	for (int i = 0; i < k; ++i) 
        tk  *= t;

	// Calculate (1-t)^(n-k)
	Real t1k = 1.0;
	for (int i = 0; i < n - k; ++i) 
        t1k *= (1.0 - t);

	return binom * tk * t1k;
}

Layer_FreeFormDeform::Layer_FreeFormDeform():
	Layer_MeshTransform(1.0, Color::BLEND_STRAIGHT),
	param_point1(ValueBase(Point(-4,  4))),
	param_point2(ValueBase(Point( 4, -4))),
	param_lattice_cols(ValueBase(4)),
	param_lattice_rows(ValueBase(4)),
	param_mesh_cols(ValueBase(16)),
	param_mesh_rows(ValueBase(16))
{
	param_control_points.set_list_of(std::vector<Point>());
	reinit_control_points();
	prepare_mesh();

	SET_INTERPOLATION_DEFAULTS();
	SET_STATIC_DEFAULTS();
}

Layer_FreeFormDeform::~Layer_FreeFormDeform()
{
}

String
Layer_FreeFormDeform::get_local_name()const
{
	String s = Layer_MeshTransform::get_local_name();
	return s.empty() ? _("Free Form Deform") : '[' + s + ']';
}

Layer::Vocab
Layer_FreeFormDeform::get_param_vocab()const
{
	Layer::Vocab ret(Layer_MeshTransform::get_param_vocab());

	ret.push_back(ParamDesc("control_points")
		.set_local_name(_("Control Points"))
		.set_description(_("Displaced positions of the control lattice, in row-major order"))
	);
	ret.push_back(ParamDesc("point1")
		.set_local_name(_("Point 1"))
		.set_box("point2")
		.set_description(_("First corner of the deformation bounding box"))
		.set_is_distance()
	);
	ret.push_back(ParamDesc("point2")
		.set_local_name(_("Point 2"))
		.set_description(_("Second corner of the deformation bounding box"))
		.set_is_distance()
	);
	ret.push_back(ParamDesc("lattice_cols")
		.set_local_name(_("Lattice Columns"))
		.set_description(_("Number of control points horizontally (Bernstein degree + 1)"))
		.set_static(true)
	);
	ret.push_back(ParamDesc("lattice_rows")
		.set_local_name(_("Lattice Rows"))
		.set_description(_("Number of control points vertically (Bernstein degree + 1)"))
		.set_static(true)
	);
	ret.push_back(ParamDesc("mesh_cols")
		.set_local_name(_("Mesh Columns"))
		.set_description(_("Number of mesh vertices horizontally (rendering quality)"))
		.set_static(true)
	);
	ret.push_back(ParamDesc("mesh_rows")
		.set_local_name(_("Mesh Rows"))
		.set_description(_("Number of mesh vertices vertically (rendering quality)"))
		.set_static(true)
	);

	return ret;
}

bool
Layer_FreeFormDeform::set_param(const String &param, const ValueBase &value)
{
	IMPORT_VALUE_PLUS(param_control_points, prepare_mesh());
	IMPORT_VALUE_PLUS(param_point1,         reinit_control_points(); prepare_mesh());
	IMPORT_VALUE_PLUS(param_point2,         reinit_control_points(); prepare_mesh());
	IMPORT_VALUE_PLUS(param_lattice_cols,   reinit_control_points(); prepare_mesh());
	IMPORT_VALUE_PLUS(param_lattice_rows,   reinit_control_points(); prepare_mesh());
	IMPORT_VALUE_PLUS(param_mesh_cols,      prepare_mesh());
	IMPORT_VALUE_PLUS(param_mesh_rows,      prepare_mesh());
	return Layer_MeshTransform::set_param(param, value);
}

ValueBase
Layer_FreeFormDeform::get_param(const String &param)const
{
	EXPORT_VALUE(param_control_points);
	EXPORT_VALUE(param_point1);
	EXPORT_VALUE(param_point2);
	EXPORT_VALUE(param_lattice_cols);
	EXPORT_VALUE(param_lattice_rows);
	EXPORT_VALUE(param_mesh_cols);
	EXPORT_VALUE(param_mesh_rows);

	EXPORT_NAME();
	EXPORT_VERSION();

	return Layer_MeshTransform::get_param(param);
}

void Layer_FreeFormDeform::reinit_control_points()
{
	const Point p0   = param_point1.get(Point());
	const Point p1   = param_point2.get(Point());
	const int cols = std::max(2, param_lattice_cols.get(int()));
	const int rows = std::max(2, param_lattice_rows.get(int()));

	const Real step_x = (p1[0] - p0[0]) / (Real)(cols - 1);
	const Real step_y = (p1[1] - p0[1]) / (Real)(rows - 1);

	std::vector<Point> pts;
	pts.reserve(cols * rows);
	for (int j = rows - 1; j >= 0; --j)
		for (int i = 0; i < cols; ++i)
			pts.push_back(Point(p0[0] + i*step_x, p0[1] + j*step_y));

	param_control_points.set_list_of(pts);
}

static std::vector<Point> extract_control_points(const ValueBase &param)
{
	std::vector<ValueBase> raw = param.get_list();
	std::vector<Point> cp;
	cp.reserve(raw.size());
	for (const ValueBase &v : raw)
		cp.push_back(v.get(Point()));
	return cp;
}

void
Layer_FreeFormDeform::prepare_mesh()
{
	rendering::Mesh::Handle mesh(new rendering::Mesh());

	const Point p0 = param_point1.get(Point());
	const Point p1 = param_point2.get(Point());

	const int lc = std::max(2, param_lattice_cols.get(int()));
	const int lr = std::max(2, param_lattice_rows.get(int()));
	const int l = lc - 1;
	const int m = lr - 1;

	const int mc = std::max(2, param_mesh_cols.get(int()));
	const int mr = std::max(2, param_mesh_rows.get(int()));

	std::vector<Point> cp = extract_control_points(param_control_points);

	// If the number of control points does not equal lattice size, re-init them to the default positions
	if ((int)cp.size() != lc * lr) {
		reinit_control_points();
		cp = extract_control_points(param_control_points);
	}

	const Real step_x = (p1[0] - p0[0]) / (Real)(mc - 1);
	const Real step_y = (p1[1] - p0[1]) / (Real)(mr - 1);

	mesh->vertices.reserve(mc * mr);
	for (int j = 0; j < mr; ++j)
	{
		for (int i = 0; i < mc; ++i)
		{
			// Calculate the position of the vertex (i,j) using the Bernstein polynomials
			Point rest(p0[0] + i*step_x, p0[1] + j*step_y);
			Real s = (Real)i / (Real)(mc - 1);
			Real t = 1.0 - (Real)j / (Real)(mr - 1);

			Vector deformed(0.0, 0.0);
			for (int ci = 0; ci < lc; ++ci)
			{
				Real bs = bernstein(l, ci, s);
				for (int cj = 0; cj < lr; ++cj)
					deformed += Vector(cp[cj*lc + ci]) * (bs * bernstein(m, cj, t));
			}
			
			mesh->vertices.push_back(rendering::Mesh::Vertex(deformed, rest));
		}
	}

	// Build two triangles for each quad in the grid
	for (int j = 1; j < mr; ++j)
	{
		for (int i = 1; i < mc; ++i)
		{
			int v[] = {
				(j-1)*mc + (i-1),
				(j-1)*mc +  i,
				j*mc +  i,
				j*mc + (i-1),
			};
			mesh->triangles.push_back(rendering::Mesh::Triangle(v[0], v[1], v[3]));
			mesh->triangles.push_back(rendering::Mesh::Triangle(v[1], v[2], v[3]));
		}
	}

	this->mesh = mesh;
}

// These 2 methods are implemented to prevent the default behavior of blending, 
// which for this layer results in drawing the deformed and undeformed version on top of each other

rendering::Task::Handle
Layer_FreeFormDeform::build_composite_fork_task_vfunc(ContextParams context_params, rendering::Task::Handle sub_task) const
{
    if (!sub_task) return rendering::Task::Handle();

    rendering::TaskMesh::Handle task_mesh(new rendering::TaskMesh());
    task_mesh->mesh = new rendering::Mesh();
    task_mesh->mesh->assign(*mesh);
    task_mesh->sub_task() = sub_task;
    return task_mesh;
}

rendering::Task::Handle
Layer_FreeFormDeform::build_rendering_task_vfunc(Context context) const
{
    rendering::Task::Handle sub_task = context.build_rendering_task();
    return build_composite_fork_task_vfunc(context.get_params(), sub_task);
}
