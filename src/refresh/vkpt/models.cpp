/*
Copyright (C) 2018 Christoph Schied
Copyright (C) 2018 Florian Simon
Copyright (C) 2003-2006 Andrey Nazarov
Copyright (C) 2019, NVIDIA CORPORATION. All rights reserved.

This program is free software; you can redistribute it and/or modify
it under the terms of the GNU General Public License as published by
the Free Software Foundation; either version 2 of the License, or
(at your option) any later version.

This program is distributed in the hope that it will be useful,
but WITHOUT ANY WARRANTY; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
GNU General Public License for more details.

You should have received a copy of the GNU General Public License along
with this program; if not, write to the Free Software Foundation, Inc.,
51 Franklin Street, Fifth Floor, Boston, MA 02110-1301 USA.
*/

/*
Copyright (C) 2003-2006 Andrey Nazarov

This program is free software; you can redistribute it and/or modify
it under the terms of the GNU General Public License as published by
the Free Software Foundation; either version 2 of the License, or
(at your option) any later version.

This program is distributed in the hope that it will be useful,
but WITHOUT ANY WARRANTY; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
GNU General Public License for more details.

You should have received a copy of the GNU General Public License along
with this program; if not, write to the Free Software Foundation, Inc.,
51 Franklin Street, Fifth Floor, Boston, MA 02110-1301 USA.
*/

#include "vkpt.h"

#include "format/md2.h"
#include "format/md3.h"
#include "format/sp2.h"
#include "material.h"
#include <assert.h>

#if MAX_ALIAS_VERTS > TESS_MAX_VERTICES
#error TESS_MAX_VERTICES
#endif

#if MD2_MAX_TRIANGLES > TESS_MAX_INDICES / 3
#error TESS_MAX_INDICES
#endif

static void computeTangents(model_t * model)
{
    for (int idx_mesh = 0; idx_mesh < model->nummeshes; ++idx_mesh)
    {
        maliasmesh_t * mesh = &model->meshes[idx_mesh];

        assert(mesh->tangents);
        float * stangents = (float*)Z_Malloc(mesh->numverts * 2 * 3 * sizeof(float)); // C++20 VKPT: Added cast
        float * ttangents = stangents + (mesh->numverts * 3);
 
        for (int idx_frame = 0; idx_frame < model->numframes; ++idx_frame)
        {
            memset(stangents, 0, mesh->numverts * 2 * 3 * sizeof(float));

            uint32_t ntriangles = mesh->numindices / 3,
                     offset = idx_frame * mesh->numverts;
            
            for (int idx_tri = 0; idx_tri < mesh->numtris; ++idx_tri)
            {
                uint32_t iA = mesh->indices[idx_tri * 3 + 0];
                uint32_t iB = mesh->indices[idx_tri * 3 + 1];
                uint32_t iC = mesh->indices[idx_tri * 3 + 2];

                float const * pA = (float const *)mesh->positions + ((offset + iA) * 3);
                float const * pB = (float const *)mesh->positions + ((offset + iB) * 3);
                float const * pC = (float const *)mesh->positions + ((offset + iC) * 3);

                float const * tA = (float const *)mesh->tex_coords + ((offset + iA) * 2);
                float const * tB = (float const *)mesh->tex_coords + ((offset + iB) * 2);
                float const * tC = (float const *)mesh->tex_coords + ((offset + iC) * 2);

                vec3_t dP0, dP1;
                VectorSubtract(pB, pA, dP0);
                VectorSubtract(pC, pA, dP1);

                vec2_t dt0, dt1;
                Vector2Subtract(tB, tA, dt0);
                Vector2Subtract(tC, tA, dt1);

                float r = 1.f / (dt0[0] * dt1[1] - dt1[0] * dt0[1]);

                vec3_t sdir = {
                    (dt1[1] * dP0[0] - dt0[1] * dP1[0]) * r,
                    (dt1[1] * dP0[1] - dt0[1] * dP1[1]) * r,
                    (dt1[1] * dP0[2] - dt0[1] * dP1[2]) * r };

                vec3_t tdir = {
                    (dt0[0] * dP1[0] - dt1[0] * dP0[0]) * r,
                    (dt0[0] * dP1[1] - dt1[0] * dP0[1]) * r,
                    (dt0[0] * dP1[2] - dt1[0] * dP0[2]) * r };

                VectorAdd(stangents + (iA * 3), sdir, stangents + (iA * 3));
                VectorAdd(stangents + (iB * 3), sdir, stangents + (iB * 3));
                VectorAdd(stangents + (iC * 3), sdir, stangents + (iC * 3));

                VectorAdd(ttangents + (iA * 3), tdir, ttangents + (iA * 3));
                VectorAdd(ttangents + (iB * 3), tdir, ttangents + (iB * 3));
                VectorAdd(ttangents + (iC * 3), tdir, ttangents + (iC * 3));
            }

            for (int idx_vert = 0; idx_vert < mesh->numverts; ++idx_vert)
            {
                float const * normal = (float const *)mesh->normals + ((offset + idx_vert) * 3);
                float const * stan = stangents + (idx_vert * 3);
                float const * ttan = ttangents + (idx_vert * 3);

                float * tangent = (float *)mesh->tangents + ((offset+idx_vert) * 4);

                vec3_t t;
                VectorScale(normal, DotProduct(normal, stan), t);
                VectorSubtract(stan, t, t);
                VectorNormalize2(t, tangent); // Graham-Schmidt : t = normalize(t - n * (n.t))

                vec3_t cross;
                CrossProduct(normal, t, cross);
                float dot = DotProduct(cross, ttan);
                tangent[3] = dot < 0.0f ? -1.0f : 1.0f; // handedness
            }
        }
        Z_Free(stangents);
    }
}

static void export_obj_frames(model_t* model, const char* path_pattern)
{
	for (int idx_frame = 0; idx_frame < model->numframes; ++idx_frame)
	{
		char path[MAX_OSPATH];
		sprintf(path, path_pattern, idx_frame);
		FILE* file = fopen(path, "w");

		if (!file)
			continue;

		int mesh_vertex_offset = 1; // obj indexing starts at 1

		for (int idx_mesh = 0; idx_mesh < model->nummeshes; ++idx_mesh)
		{
			maliasmesh_t * mesh = &model->meshes[idx_mesh];
			uint32_t ntriangles = mesh->numindices / 3,
				offset = idx_frame * mesh->numverts;

			for (int idx_vert = 0; idx_vert < mesh->numverts; ++idx_vert)
			{
				float const * p = (float const*)mesh->positions + (offset + idx_vert) * 3;
				float const * n = (float const*)mesh->normals + (offset + idx_vert) * 3;
				float const * t = (float const*)mesh->tex_coords + (offset + idx_vert) * 2;
				fprintf(file, "v %.3f %.3f %.3f\n", p[0], p[1], p[2]);
				fprintf(file, "vn %.3f %.3f %.3f\n", n[0], n[1], n[2]);
				fprintf(file, "vt %.3f %.3f\n", t[0], t[1]);
			}

			fprintf(file, "g mesh_%d\n", idx_mesh);

			for (int idx_tri = 0; idx_tri < mesh->numtris; ++idx_tri)
			{
				int iA = mesh->indices[idx_tri * 3 + 0] + mesh_vertex_offset;
				int iB = mesh->indices[idx_tri * 3 + 1] + mesh_vertex_offset;
				int iC = mesh->indices[idx_tri * 3 + 2] + mesh_vertex_offset;

				fprintf(file, "f %d/%d/%d %d/%d/%d %d/%d/%d\n", iA, iA, iA, iB, iB, iB, iC, iC, iC);
			}

			mesh_vertex_offset += mesh->numverts;
		}

		fclose(file);
	}
}

qerror_t MOD_LoadMD2_RTX(model_t *model, const void *rawdata, size_t length)
{
	dmd2header_t    header;
	dmd2frame_t     *src_frame;
	dmd2trivertx_t  *src_vert;
	dmd2triangle_t  *src_tri;
	dmd2stvert_t    *src_tc;
	char            *src_skin;
	maliasframe_t   *dst_frame;
	maliasmesh_t    *dst_mesh;
	int             val;
	uint16_t        remap[TESS_MAX_INDICES];
	uint16_t        vertIndices[TESS_MAX_INDICES];
	uint16_t        tcIndices[TESS_MAX_INDICES];
	uint16_t        finalIndices[TESS_MAX_INDICES];
	int             numverts, numindices;
	char            skinname[MAX_QPATH];
	vec_t           scale_s, scale_t;
	vec3_t          mins, maxs;
	qerror_t        ret;

	if (length < sizeof(header)) {
		return Q_ERR_FILE_TOO_SMALL;
	}

	// byte swap the header
	header = *(dmd2header_t *)rawdata;
	for (int i = 0; i < sizeof(header) / 4; i++) {
		((uint32_t *)&header)[i] = LittleLong(((uint32_t *)&header)[i]);
	}

	// validate the header
	ret = MOD_ValidateMD2(&header, length);
	if (ret) {
		if (ret == Q_ERR_TOO_FEW) {
			// empty models draw nothing
			model->type = model_t::MOD_EMPTY; // C++20 VKPT: Added enum
			return Q_ERR_SUCCESS;
		}
		return ret;
	}

	// load all triangle indices
	numindices = 0;
	src_tri = (dmd2triangle_t *)((byte *)rawdata + header.ofs_tris);
	for (int i = 0; i < header.num_tris; i++) {
		int good = 1;
		for (int j = 0; j < 3; j++) {
			uint16_t idx_xyz = LittleShort(src_tri->index_xyz[j]);
			uint16_t idx_st = LittleShort(src_tri->index_st[j]);

			// some broken models have 0xFFFF indices
			if (idx_xyz >= header.num_xyz || idx_st >= header.num_st) {
				good = 0;
				break;
			}

			vertIndices[numindices + j] = idx_xyz;
			tcIndices[numindices + j] = idx_st;
		}
		if (good) {
			// only count good triangles
			numindices += 3;
		}
		src_tri++;
	}

	if (numindices < 3) {
		return Q_ERR_TOO_FEW;
	}

	qboolean all_normals_same = qtrue;
	int same_normal = -1;

	src_frame = (dmd2frame_t *)((byte *)rawdata + header.ofs_frames);
	for (int i = 0; i < numindices; i++)
	{
		int v = vertIndices[i];
		int normal = src_frame->verts[v].lightnormalindex;

		// detect if the model has broken normals - they are all the same in that case
		// it happens with players/w_<weapon>.md2 models for example
		if (same_normal < 0)
			same_normal = normal;
		else if (normal != same_normal)
			all_normals_same = qfalse;
	}

	for (int i = 0; i < numindices; i++) {
		remap[i] = 0xFFFF;
	}

	// remap all triangle indices
	numverts = 0;
	src_tc = (dmd2stvert_t *)((byte *)rawdata + header.ofs_st);
	for (int i = 0; i < numindices; i++) {
		if (remap[i] != 0xFFFF) {
			continue; // already remapped
		}

		// only dedup vertices if we're not regenerating normals
		if (!all_normals_same)
		{
			for (int j = i + 1; j < numindices; j++) {
				if (vertIndices[i] == vertIndices[j] &&
					(src_tc[tcIndices[i]].s == src_tc[tcIndices[j]].s &&
						src_tc[tcIndices[i]].t == src_tc[tcIndices[j]].t)) {
					// duplicate vertex
					remap[j] = i;
					finalIndices[j] = numverts;
				}
			}
		}

		// new vertex
		remap[i] = i;
		finalIndices[i] = numverts++;
	}

	Hunk_Begin(&model->hunk, 50u<<20);
	model->type = model_t::MOD_ALIAS; // C++20 VKPT: Added enum
	model->nummeshes = 1;
	model->numframes = header.num_frames;
	model->meshes = (maliasmesh_s*)MOD_Malloc(sizeof(maliasmesh_t)); // C++20 VKPT: Added cast.
	model->frames = (maliasframe_s*)MOD_Malloc(header.num_frames * sizeof(maliasframe_t)); // C++20 VKPT: Added cast.

	dst_mesh = model->meshes;
	dst_mesh->numtris    = numindices / 3;
	dst_mesh->numindices = numindices;
	dst_mesh->numverts   = numverts;
	dst_mesh->numskins   = header.num_skins;
	dst_mesh->positions  = (vec3_t*)MOD_Malloc(numverts   * header.num_frames * sizeof(vec3_t)); // C++20 VKPT: Added cast.
	dst_mesh->normals    = (vec3_t*)MOD_Malloc(numverts   * header.num_frames * sizeof(vec3_t)); // C++20 VKPT: Added cast.
	dst_mesh->tex_coords = (vec2_t*)MOD_Malloc(numverts   * header.num_frames * sizeof(vec2_t)); // C++20 VKPT: Added cast.
    dst_mesh->tangents   = (vec4_t*)MOD_Malloc(numverts   * header.num_frames * sizeof(vec4_t)); // C++20 VKPT: Added cast.
	dst_mesh->indices    = (int*)MOD_Malloc(numindices * sizeof(int));

	if (dst_mesh->numtris != header.num_tris) {
		Com_DPrintf("%s has %d bad triangles\n", model->name, header.num_tris - dst_mesh->numtris);
	}

	// store final triangle indices
	for (int i = 0; i < numindices; i++) {
		dst_mesh->indices[i] = finalIndices[i];
	}

	// load all skins
	src_skin = (char *)rawdata + header.ofs_skins;
	for (int i = 0; i < header.num_skins; i++) {
		if (!Q_memccpy(skinname, src_skin, 0, sizeof(skinname))) {
			ret = Q_ERR_STRING_TRUNCATED;
			goto fail;
		}
		FS_NormalizePath(skinname, skinname);

		pbr_material_t * mat = MAT_FindPBRMaterial(skinname);
		if (!mat)
			Com_EPrintf("error finding material '%s'\n", skinname);

		image_t* image_diffuse = IMG_Find(skinname, IT_SKIN, IF_SRGB);
		image_t* image_normals = NULL;
		image_t* image_emissive = NULL;

		if (image_diffuse != R_NOTEXTURE)
		{
			// attempt loading the normals texture
			if (!Q_strlcpy(skinname, src_skin, strlen(src_skin) - 3))
				return Q_ERR_STRING_TRUNCATED;

			Q_concat(skinname, sizeof(skinname), skinname, "_n.tga", NULL);
			FS_NormalizePath(skinname, skinname);
			image_normals = IMG_Find(skinname, IT_SKIN, IF_NONE);
			if (image_normals == R_NOTEXTURE) image_normals = NULL;

			// attempt loading the emissive texture
			if (!Q_strlcpy(skinname, src_skin, strlen(src_skin) - 3))
				return Q_ERR_STRING_TRUNCATED;

			Q_concat(skinname, sizeof(skinname), skinname, "_light.tga", NULL);
			FS_NormalizePath(skinname, skinname);
			image_emissive = IMG_Find(skinname, IT_SKIN, IF_SRGB);
			if (image_emissive == R_NOTEXTURE) image_emissive = NULL;
		}

		MAT_RegisterPBRMaterial(mat, image_diffuse, image_normals, image_emissive);

		dst_mesh->materials[i] = mat;

        src_skin += MD2_MAX_SKINNAME;
	}

	// load all tcoords
	src_tc = (dmd2stvert_t *)((byte *)rawdata + header.ofs_st);
	scale_s = 1.0f / header.skinwidth;
	scale_t = 1.0f / header.skinheight;

	// load all frames
	src_frame = (dmd2frame_t *)((byte *)rawdata + header.ofs_frames);
	dst_frame = model->frames;
	for (int j = 0; j < header.num_frames; j++) {
		LittleVector(src_frame->scale, dst_frame->scale);
		LittleVector(src_frame->translate, dst_frame->translate);

		// load frame vertices
		ClearBounds(mins, maxs);

		for (int i = 0; i < numindices; i++) {
			if (remap[i] != i) {
				continue;
			}
			src_vert = &src_frame->verts[vertIndices[i]];
			vec3_t *dst_pos = &dst_mesh->positions [j * numverts + finalIndices[i]];
			vec3_t *dst_nrm = &dst_mesh->normals   [j * numverts + finalIndices[i]];
			vec2_t *dst_tc  = &dst_mesh->tex_coords[j * numverts + finalIndices[i]];

			(*dst_tc)[0] = scale_s * src_tc[tcIndices[i]].s;
			(*dst_tc)[1] = scale_t * src_tc[tcIndices[i]].t;

			(*dst_pos)[0] = src_vert->v[0] * dst_frame->scale[0] + dst_frame->translate[0];
			(*dst_pos)[1] = src_vert->v[1] * dst_frame->scale[1] + dst_frame->translate[1];
			(*dst_pos)[2] = src_vert->v[2] * dst_frame->scale[2] + dst_frame->translate[2];

			(*dst_nrm)[0] = 0.0f;
			(*dst_nrm)[1] = 0.0f;
			(*dst_nrm)[2] = 0.0f;

			val = src_vert->lightnormalindex;

			if (val < NUMVERTEXNORMALS) {
				(*dst_nrm)[0] = bytedirs[val][0];
				(*dst_nrm)[1] = bytedirs[val][1];
				(*dst_nrm)[2] = bytedirs[val][2];
			}

			for (int k = 0; k < 3; k++) {
				val = (*dst_pos)[k];
				if (val < mins[k])
					mins[k] = val;
				if (val > maxs[k])
					maxs[k] = val;
			}
		}

		// if all normals are the same, rebuild them as flat triangle normals
		if (all_normals_same)
		{
			for (int tri = 0; tri < numindices / 3; tri++)
			{
				int i0 = j * numverts + finalIndices[tri * 3 + 0];
				int i1 = j * numverts + finalIndices[tri * 3 + 1];
				int i2 = j * numverts + finalIndices[tri * 3 + 2];

				vec3_t *p0 = &dst_mesh->positions[i0];
				vec3_t *p1 = &dst_mesh->positions[i1];
				vec3_t *p2 = &dst_mesh->positions[i2];

				vec3_t e1, e2, n;
				VectorSubtract(*p1, *p0, e1);
				VectorSubtract(*p2, *p0, e2);
				CrossProduct(e2, e1, n);
				VectorNormalize(n);

				VectorCopy(n, dst_mesh->normals[i0]);
				VectorCopy(n, dst_mesh->normals[i1]);
				VectorCopy(n, dst_mesh->normals[i2]);
			}
		}

		VectorVectorScale(mins, dst_frame->scale, mins);
		VectorVectorScale(maxs, dst_frame->scale, maxs);

		dst_frame->radius = RadiusFromBounds(mins, maxs);

		VectorAdd(mins, dst_frame->translate, dst_frame->bounds[0]);
		VectorAdd(maxs, dst_frame->translate, dst_frame->bounds[1]);

		src_frame = (dmd2frame_t *)((byte *)src_frame + header.framesize);
		dst_frame++;
	}

	// fix winding order
	for (int i = 0; i < dst_mesh->numindices; i += 3) {
		int tmp = dst_mesh->indices[i + 1];
		dst_mesh->indices[i + 1] = dst_mesh->indices[i + 2];
		dst_mesh->indices[i + 2] = tmp;
	}

    computeTangents(model);

	Hunk_End(&model->hunk);
	return Q_ERR_SUCCESS;

fail:
	Hunk_Free(&model->hunk);
	return ret;
}

#if USE_MD3

#define TAB_SIN(x) qvk.sintab[(x) & 255]
#define TAB_COS(x) qvk.sintab[((x) + 64) & 255]

static qerror_t MOD_LoadMD3Mesh(model_t *model, maliasmesh_t *mesh,
		const byte *rawdata, size_t length, size_t *offset_p)
{
	dmd3mesh_t      header;
	size_t          end;
	dmd3vertex_t    *src_vert;
	dmd3coord_t     *src_tc;
	dmd3skin_t      *src_skin;
	uint32_t        *src_idx;
	vec3_t          *dst_vert;
	vec3_t          *dst_norm;
	vec2_t          *dst_tc;
    vec4_t          *dst_tan;
	int  *dst_idx;
	char            skinname[MAX_QPATH];
	int             i;

	if (length < sizeof(header))
		return Q_ERR_BAD_EXTENT;

	// byte swap the header
	header = *(dmd3mesh_t *)rawdata;
	for (i = 0; i < sizeof(header) / 4; i++)
		((uint32_t *)&header)[i] = LittleLong(((uint32_t *)&header)[i]);

	if (header.meshsize < sizeof(header) || header.meshsize > length)
		return Q_ERR_BAD_EXTENT;
	if (header.num_verts < 3)
		return Q_ERR_TOO_FEW;
	if (header.num_verts > TESS_MAX_VERTICES)
		return Q_ERR_TOO_MANY;
	if (header.num_tris < 1)
		return Q_ERR_TOO_FEW;
	if (header.num_tris > TESS_MAX_INDICES / 3)
		return Q_ERR_TOO_MANY;
	if (header.num_skins > MAX_ALIAS_SKINS)
		return Q_ERR_TOO_MANY;
	end = header.ofs_skins + header.num_skins * sizeof(dmd3skin_t);
	if (end < header.ofs_skins || end > length)
		return Q_ERR_BAD_EXTENT;
	end = header.ofs_verts + header.num_verts * model->numframes * sizeof(dmd3vertex_t);
	if (end < header.ofs_verts || end > length)
		return Q_ERR_BAD_EXTENT;
	end = header.ofs_tcs + header.num_verts * sizeof(dmd3coord_t);
	if (end < header.ofs_tcs || end > length)
		return Q_ERR_BAD_EXTENT;
	end = header.ofs_indexes + header.num_tris * 3 * sizeof(uint32_t);
	if (end < header.ofs_indexes || end > length)
		return Q_ERR_BAD_EXTENT;

	mesh->numtris = header.num_tris;
	mesh->numindices = header.num_tris * 3;
	mesh->numverts = header.num_verts;
	mesh->numskins = header.num_skins;
	mesh->positions = (vec3_t*)MOD_Malloc(header.num_verts * model->numframes * sizeof(vec3_t)); // C++20 VKPT: Added cast.
	mesh->normals = (vec3_t*)MOD_Malloc(header.num_verts * model->numframes * sizeof(vec3_t)); // C++20 VKPT: Added cast.
	mesh->tex_coords = (vec2_t*)MOD_Malloc(header.num_verts * model->numframes * sizeof(vec2_t)); // C++20 VKPT: Added cast.
    mesh->tangents = (vec4_t*)MOD_Malloc(header.num_verts * header.num_frames * sizeof(vec4_t)); // C++20 VKPT: Added cast.
	mesh->indices = (int*)MOD_Malloc(sizeof(int) * header.num_tris * 3); // C++20 VKPT: Added cast.

	// load all skins
	src_skin = (dmd3skin_t *)(rawdata + header.ofs_skins);
	for (i = 0; i < header.num_skins; i++, src_skin++) {
		if (!Q_memccpy(skinname, src_skin->name, 0, sizeof(skinname)))
			return Q_ERR_STRING_TRUNCATED;
		FS_NormalizePath(skinname, skinname);

		pbr_material_t * mat = MAT_FindPBRMaterial(skinname);
		if (!mat)
			Com_EPrintf("error finding material '%s'\n", skinname);

		image_t* image_diffuse = IMG_Find(skinname, IT_SKIN, IF_SRGB);
		image_t* image_normals = NULL;
		image_t* image_emissive = NULL;

		if (image_diffuse != R_NOTEXTURE)
		{
			// attempt loading the normals texture
			if (!Q_strlcpy(skinname, src_skin->name, strlen(src_skin->name) - 3))
				return Q_ERR_STRING_TRUNCATED;

			Q_concat(skinname, sizeof(skinname), skinname, "_n.tga", NULL);
			FS_NormalizePath(skinname, skinname);
			image_normals = IMG_Find(skinname, IT_SKIN, IF_NONE);
			if (image_normals == R_NOTEXTURE) image_normals = NULL;

			// attempt loading the emissive texture
			if (!Q_strlcpy(skinname, src_skin->name, strlen(src_skin->name) - 3))
				return Q_ERR_STRING_TRUNCATED;

			Q_concat(skinname, sizeof(skinname), skinname, "_light.tga", NULL);
			FS_NormalizePath(skinname, skinname);
			image_emissive = IMG_Find(skinname, IT_SKIN, IF_SRGB);
			if (image_emissive == R_NOTEXTURE) image_emissive = NULL;
		}

		MAT_RegisterPBRMaterial(mat, image_diffuse, image_normals, image_emissive);

		mesh->materials[i] = mat;
    }

	// load all vertices
	src_vert = (dmd3vertex_t *)(rawdata + header.ofs_verts);
	dst_vert = mesh->positions;
	dst_norm = mesh->normals;
	dst_tc = mesh->tex_coords;
    dst_tan = mesh->tangents;
	for (int frame = 0; frame < header.num_frames; frame++)
	{
		src_tc = (dmd3coord_t *)(rawdata + header.ofs_tcs);

		for (i = 0; i < header.num_verts; i++) 
		{
			(*dst_vert)[0] = (float)(src_vert->point[0]) / 64.f;
			(*dst_vert)[1] = (float)(src_vert->point[1]) / 64.f;
			(*dst_vert)[2] = (float)(src_vert->point[2]) / 64.f;

			unsigned int lat = src_vert->norm[0];
			unsigned int lng = src_vert->norm[1];

			(*dst_norm)[0] = TAB_SIN(lat) * TAB_COS(lng);
			(*dst_norm)[1] = TAB_SIN(lat) * TAB_SIN(lng);
			(*dst_norm)[2] = TAB_COS(lat);

			VectorNormalize(*dst_norm);

			(*dst_tc)[0] = LittleFloat(src_tc->st[0]);
			(*dst_tc)[1] = LittleFloat(src_tc->st[1]);

            (*dst_tan)[0] = 0.0f;
            (*dst_tan)[1] = 0.0f;
            (*dst_tan)[2] = 0.0f;
            (*dst_tan)[3] = 0.0f;

			src_vert++; dst_vert++; dst_norm++;
            src_tc++; dst_tc++; dst_tan++;
		}
	}


	// load all triangle indices
	src_idx = (uint32_t *)(rawdata + header.ofs_indexes);
	dst_idx = mesh->indices;
	for (i = 0; i < header.num_tris; i++) 
	{
		dst_idx[0] = LittleLong(src_idx[2]);
		dst_idx[1] = LittleLong(src_idx[1]);
		dst_idx[2] = LittleLong(src_idx[0]);

		if (dst_idx[0] >= header.num_verts)
			return Q_ERR_BAD_INDEX;

		src_idx += 3;
		dst_idx += 3;
	}
	
	*offset_p = header.meshsize;

	return Q_ERR_SUCCESS;
}

qerror_t MOD_LoadMD3_RTX(model_t *model, const void *rawdata, size_t length)
{
	dmd3header_t    header;
	size_t          end, offset, remaining;
	dmd3frame_t     *src_frame;
	maliasframe_t   *dst_frame;
	const byte      *src_mesh;
	int             i;
	qerror_t        ret;

	if (length < sizeof(header))
		return Q_ERR_FILE_TOO_SMALL;

	// byte swap the header
	header = *(dmd3header_t *)rawdata;
	for (i = 0; i < sizeof(header) / 4; i++)
		((uint32_t *)&header)[i] = LittleLong(((uint32_t *)&header)[i]);

	if (header.ident != MD3_IDENT)
		return Q_ERR_UNKNOWN_FORMAT;
	if (header.version != MD3_VERSION)
		return Q_ERR_UNKNOWN_FORMAT;
	if (header.num_frames < 1)
		return Q_ERR_TOO_FEW;
	if (header.num_frames > MD3_MAX_FRAMES)
		return Q_ERR_TOO_MANY;
	end = header.ofs_frames + sizeof(dmd3frame_t) * header.num_frames;
	if (end < header.ofs_frames || end > length)
		return Q_ERR_BAD_EXTENT;
	if (header.num_meshes < 1)
		return Q_ERR_TOO_FEW;
	if (header.num_meshes > MD3_MAX_MESHES)
		return Q_ERR_TOO_MANY;
	if (header.ofs_meshes > length)
		return Q_ERR_BAD_EXTENT;

	Hunk_Begin(&model->hunk, 0x4000000);
	model->type = model_t::MOD_ALIAS; // C++20 VKPT: Added enum
	model->numframes = header.num_frames;
	model->nummeshes = header.num_meshes;
	model->meshes = (maliasmesh_s*)MOD_Malloc(sizeof(maliasmesh_t) * header.num_meshes); // C++20 VKPT: Added cast
	model->frames = (maliasframe_s*)MOD_Malloc(sizeof(maliasframe_t) * header.num_frames); // C++20 VKPT: Added cast

	// load all frames
	src_frame = (dmd3frame_t *)((byte *)rawdata + header.ofs_frames);
	dst_frame = model->frames;
	for (i = 0; i < header.num_frames; i++) {
		LittleVector(src_frame->translate, dst_frame->translate);
		VectorSet(dst_frame->scale, MD3_XYZ_SCALE, MD3_XYZ_SCALE, MD3_XYZ_SCALE);

		LittleVector(src_frame->mins, dst_frame->bounds[0]);
		LittleVector(src_frame->maxs, dst_frame->bounds[1]);
		dst_frame->radius = LittleFloat(src_frame->radius);

		src_frame++; dst_frame++;
	}

	// load all meshes
	src_mesh = (const byte *)rawdata + header.ofs_meshes;
	remaining = length - header.ofs_meshes;
	for (i = 0; i < header.num_meshes; i++) {
		ret = MOD_LoadMD3Mesh(model, &model->meshes[i], src_mesh, remaining, &offset);
		if (ret)
			goto fail;
		src_mesh += offset;
		remaining -= offset;
	}

    computeTangents(model);

	//if (strstr(model->name, "v_blast"))
	//	export_obj_frames(model, "export/v_blast_%d.obj");

	Hunk_End(&model->hunk);
	return Q_ERR_SUCCESS;

fail:
	Hunk_Free(&model->hunk);
	return ret;
}
#endif

void MOD_Reference_RTX(model_t *model)
{
	int mesh_idx, skin_idx, frame_idx;

	// register any images used by the models
	switch (model->type) {
	case model_t::MOD_ALIAS: // C++20 VKPT: Added enum.
		for (mesh_idx = 0; mesh_idx < model->nummeshes; mesh_idx++) {
			maliasmesh_t *mesh = &model->meshes[mesh_idx];
			for (skin_idx = 0; skin_idx < mesh->numskins; skin_idx++) {
				MAT_UpdateRegistration(mesh->materials[skin_idx]);
			}
		}
		break;
	case model_t::MOD_SPRITE: // C++20 VKPT: Added enum.
		for (frame_idx = 0; frame_idx < model->numframes; frame_idx++) {
			model->spriteframes[frame_idx].image->registration_sequence = registration_sequence;
		}
		break;
	case model_t::MOD_EMPTY: // C++20 VKPT: Added enum.
		break;
	default:
		Com_Error(ERR_FATAL, "%s: bad model type", __func__);
	}

	model->registration_sequence = registration_sequence;
}

// vim: shiftwidth=4 noexpandtab tabstop=4 cindent
