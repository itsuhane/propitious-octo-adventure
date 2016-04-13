#pragma once

/*
    A simple remeshing code.
    Written and tested with OpenMesh 3.2
*/

#include <vector>
#include <queue>
#include <iostream>
#include <OpenMesh/Core/IO/MeshIO.hh>
#include <OpenMesh/Core/Mesh/TriMesh_ArrayKernelT.hh>

class remesher {
public:
    typedef OpenMesh::TriMesh_ArrayKernelT<> mesh_type;

    remesher(mesh_type &mesh) : mesh(mesh) {
        if (!test_manifold()) {
            std::cout << "Mesh is not 2-manifold, remesher may run into error." << std::endl;
            return;
        }
        prepare_valence();
        prepare_normal();
        prepare_status();
    }

    void remesh(float target_edge_len, size_t max_iter) {
        float edge_len_lo = 0.8f*target_edge_len;
        float edge_len_hi = 4.0f / 3.0f*target_edge_len;

        mesh.add_property(npoint_ph);

        for (size_t n_iter = 0; n_iter < max_iter; ++n_iter) {
            split_longer(edge_len_hi*edge_len_hi);
            collapse_shorter(edge_len_lo*edge_len_lo, edge_len_hi*edge_len_hi);
            adjust_valence();
            smooth(5, 0.2f, true);
            force_gc();
        }

        smooth(2, 0.5f, false);
        mesh.remove_property(npoint_ph);
        force_gc();
    }

private:
    struct weighted_edge {
        mesh_type::EdgeHandle handle;
        float weight;

        weighted_edge(const mesh_type::EdgeHandle &handle, float weight) : handle(handle), weight(weight) {}

        bool operator< (const weighted_edge &e) const {
            return weight < e.weight;
        }
    };

    void split_longer(float target_weight) {
        std::priority_queue<weighted_edge, std::vector<weighted_edge>> edge_to_split;

        for (mesh_type::EdgeIter e_it = mesh.edges_sbegin(); e_it != mesh.edges_end(); ++e_it) {
            float weight = edge_weight(*e_it);
            if (weight > target_weight) {
                edge_to_split.emplace(*e_it, weight);
            }
        }

        while (!edge_to_split.empty()) {
            weighted_edge e = edge_to_split.top();
            edge_to_split.pop();
            // split
            mesh_type::HalfedgeHandle heh = mesh.halfedge_handle(e.handle, 0);
            mesh_type::VertexHandle vh_from = mesh.from_vertex_handle(heh);
            mesh_type::VertexHandle vh_to = mesh.to_vertex_handle(heh);
            mesh_type::Point pt_from = mesh.point(vh_from);
            mesh_type::Point pt_to = mesh.point(vh_to);

            mesh_type::Point pt_mid = 0.5f*(pt_from + pt_to);
            mesh_type::VertexHandle vh_mid = mesh.split(e.handle, pt_mid);

            // update valence
            mesh.property(valence_ph, vh_mid) = mesh.is_boundary(vh_mid) ? 3 : 4;
            for (mesh_type::VertexVertexIter vv_it = mesh.vv_begin(vh_mid); vv_it != mesh.vv_end(vh_mid); ++vv_it) {
                if ((*vv_it != vh_from) && (*vv_it != vh_to)) {
                    mesh.property(valence_ph, *vv_it)++;
                }
            }

            // update normal
            mesh.set_normal(vh_mid, mesh.calc_vertex_normal(vh_mid));
            for (mesh_type::VertexFaceIter vf_it = mesh.vf_begin(vh_mid); vf_it != mesh.vf_end(vh_mid); ++vf_it) {
                mesh.set_normal(*vf_it, mesh.calc_face_normal(*vf_it));
            }

            // add new edges if heavier than weight
            for (mesh_type::VertexEdgeIter ve_it = mesh.ve_begin(vh_mid); ve_it != mesh.ve_end(vh_mid); ++ve_it) {
                float weight = edge_weight(*ve_it);
                if (weight > target_weight) {
                    edge_to_split.emplace(*ve_it, weight);
                }
            }
        }
    }

    void collapse_shorter(float target_weight_lo, float target_weight_hi) {
        std::vector<weighted_edge> edge_to_collapse;
        for (mesh_type::EdgeIter e_it = mesh.edges_sbegin(); e_it != mesh.edges_end(); ++e_it) {
            float weight = edge_weight(*e_it);
            if (weight < target_weight_lo) {
                edge_to_collapse.emplace_back(*e_it, weight);
            }
        }
        for (size_t i = 0; i < edge_to_collapse.size(); ++i) {
            mesh_type::HalfedgeHandle heh = mesh.halfedge_handle(edge_to_collapse[i].handle, 0);
            mesh_type::VertexHandle vh_from = mesh.from_vertex_handle(heh);
            mesh_type::VertexHandle vh_to = mesh.to_vertex_handle(heh);

            bool can_collapse = mesh.is_collapse_ok(heh) && !mesh.is_boundary(vh_from) && !mesh.is_boundary(vh_to);
            if (can_collapse) {
                for (mesh_type::VertexOHalfedgeIter voh_it(mesh, vh_from); voh_it.is_valid(); ++voh_it) {
                    if (edge_weight(mesh.edge_handle(*voh_it)) >= target_weight_hi) {
                        can_collapse = false;
                        break;
                    }
                }
            }

            if (can_collapse) {
                mesh.property(valence_ph, mesh.opposite_vh(heh))--;
                mesh.property(valence_ph, mesh.opposite_vh(mesh.opposite_halfedge_handle(heh)))--;
                mesh.collapse(heh);
                mesh.property(valence_ph, vh_to) = mesh.valence(vh_to);
            }
        }
    }

    void adjust_valence() {
        for (mesh_type::EdgeIter e_it = mesh.edges_sbegin(); e_it != mesh.edges_end(); ++e_it) {
            if (!mesh.is_flip_ok(*e_it) || mesh.is_boundary(*e_it)) continue;

            /*
                     a1
                     +
                   /   \
                 /       \
            a2 +---------->+ a0
                 \  heh  /
                   \   /
                     +
                     a3
            */

            mesh_type::HalfedgeHandle heh = mesh.halfedge_handle(*e_it, 0);
            mesh_type::VertexHandle a0 = mesh.to_vertex_handle(heh);
            mesh_type::VertexHandle a1 = mesh.opposite_vh(heh);
            mesh_type::VertexHandle a2 = mesh.from_vertex_handle(heh);
            mesh_type::VertexHandle a3 = mesh.opposite_vh(mesh.opposite_halfedge_handle(heh));

            int diff_a0 = valence(a0) - target_valence(a0);
            int diff_a1 = valence(a1) - target_valence(a1);
            int diff_a2 = valence(a2) - target_valence(a2);
            int diff_a3 = valence(a3) - target_valence(a3);

            int dev_pre = abs(diff_a0) + abs(diff_a1) + abs(diff_a2) + abs(diff_a3);
            int dev_post = abs(diff_a0 - 1) + abs(diff_a1 + 1) + abs(diff_a2 - 1) + abs(diff_a3 + 1);

            if (dev_post < dev_pre) {
                mesh.flip(*e_it);
                mesh.property(valence_ph, a0)--;
                mesh.property(valence_ph, a1)++;
                mesh.property(valence_ph, a2)--;
                mesh.property(valence_ph, a3)++;
            }
        }
    }

    void smooth(size_t max_iter, float lambda, bool tangential) {
        for (size_t n_iter = 0; n_iter < max_iter; ++n_iter) {
            for (mesh_type::VertexIter v_it = mesh.vertices_sbegin(); v_it != mesh.vertices_end(); ++v_it) {
                mesh_type::Point opoint = mesh.point(*v_it);
                if (mesh.is_boundary(*v_it)) {
                    mesh.property(npoint_ph, *v_it) = opoint;
                }
                else {
                    mesh_type::Point npoint(0.0f, 0.0f, 0.0f);
                    for (mesh_type::VertexVertexIter vv_it = mesh.vv_begin(*v_it); vv_it != mesh.vv_end(*v_it); ++vv_it) {
                        npoint += mesh.point(*vv_it);
                    }
                    npoint /= (float)valence(*v_it);
                    mesh_type::Point shift = npoint - opoint;
                    mesh_type::Point normal = mesh.normal(*v_it);
                    if (tangential) {
                        shift -= normal*dot(normal, shift);
                    }
                    mesh.property(npoint_ph, *v_it) = opoint + lambda*shift;
                }
            }
            for (mesh_type::VertexIter v_it = mesh.vertices_sbegin(); v_it != mesh.vertices_end(); ++v_it) {
                mesh.set_point(*v_it, mesh.property(npoint_ph, *v_it));
            }
        }
    }

    bool test_manifold() {
        for (mesh_type::VertexIter v_it = mesh.vertices_begin(); v_it != mesh.vertices_end(); ++v_it) {
            if (!mesh.is_manifold(*v_it)) {
                return false;
            }
        }
        return true;
    }

    void prepare_valence() {
        mesh.add_property(valence_ph);
        for (mesh_type::VertexIter v_it = mesh.vertices_begin(); v_it != mesh.vertices_end(); ++v_it) {
            mesh.property(valence_ph, *v_it) = mesh.valence(*v_it);
        }
    }

    void prepare_normal() {
        mesh.request_vertex_normals();
        mesh.request_face_normals();
        mesh.update_normals();
    }

    void prepare_status() {
        mesh.request_vertex_status();
        mesh.request_edge_status();
        mesh.request_face_status();
    }

    void force_gc() {
        mesh.garbage_collection();
    }

    int valence(const mesh_type::VertexHandle &vh) const {
        return mesh.property(valence_ph, vh);
    }

    int target_valence(const mesh_type::VertexHandle &vh) const {
        return mesh.is_boundary(vh) ? 4 : 6;
    }

    float edge_weight(const mesh_type::EdgeHandle &eh) const {
        return mesh.calc_edge_sqr_length(eh);
    }

    mesh_type &mesh;
    OpenMesh::VPropHandleT<int> valence_ph;
    OpenMesh::VPropHandleT<mesh_type::Point> npoint_ph;
    OpenMesh::VPropHandleT<float> weight_ph;
};