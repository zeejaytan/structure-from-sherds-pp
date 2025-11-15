#include "auto_agglomerative_assembler.h"
#include <iostream>
#include <iomanip>
#include <sstream>
#include <cmath>
#include <limits>
#include <random>

//==================== Constructor / Destructor ====================//

AutoAgglomerativeAssembler::AutoAgglomerativeAssembler(const Config& config)
    : config_(config), total_quality_evaluations_(0), cache_hits_(0), cache_misses_(0) {

    std::cout << "*** AUTO-AGGLOMERATIVE ASSEMBLER *** Initialized with production configuration" << std::endl;

    if (config_.enable_detailed_logging) {
        std::cout << "Configuration: max_iterations=" << config_.max_agglomerative_iterations
                  << ", refinement_iterations=" << config_.max_refinement_iterations
                  << ", convergence_threshold=" << config_.convergence_threshold << std::endl;
    }
}

AutoAgglomerativeAssembler::~AutoAgglomerativeAssembler() {
    if (config_.enable_detailed_logging && total_quality_evaluations_ > 0) {
        std::cout << "*** ASSEMBLER STATISTICS *** Quality evaluations: " << total_quality_evaluations_
                  << ", Cache hits: " << cache_hits_ << ", Cache misses: " << cache_misses_
                  << ", Hit ratio: " << std::fixed << std::setprecision(2)
                  << (100.0 * cache_hits_ / total_quality_evaluations_) << "%" << std::endl;
    }
}

//==================== Initialization Functions ====================//

std::vector<AutoAgglomerativeAssembler::FragmentCluster>
AutoAgglomerativeAssembler::initializeClusters(int total_pieces, const std::vector<LCSIndex>& all_connections) {

    std::cout << "*** PUZZLEFUSION++ INITIALIZATION *** Creating initial clusters with anchor strategy" << std::endl;

    std::vector<FragmentCluster> clusters;
    clusters.reserve(total_pieces);

    // STEP 1: ANCHOR FRAGMENT SELECTION (PuzzleFusion++ Key Innovation)
    // Select anchor fragment to resolve 3D transformation ambiguity
    int anchor_piece_id = selectAnchorFragment(all_connections, total_pieces);

    if (config_.enable_detailed_logging) {
        std::cout << "*** ANCHOR FRAGMENT *** Selected piece " << anchor_piece_id
                  << " as coordinate system anchor" << std::endl;
    }

    // STEP 2: Initialize individual fragment clusters
    for (int piece_id = 1; piece_id <= total_pieces; ++piece_id) {
        FragmentCluster cluster;
        cluster.piece_ids.push_back(piece_id);

        // ANCHOR STRATEGY: Anchor fragment gets identity transform (defines coordinate system)
        if (piece_id == anchor_piece_id) {
            cluster.transformations.push_back(Matrix4d::Identity());

            if (config_.enable_detailed_logging) {
                std::cout << "*** ANCHOR TRANSFORM *** Piece " << piece_id
                          << " initialized with identity transform (coordinate origin)" << std::endl;
            }
        } else {
            // NON-ANCHOR FRAGMENTS: Find best connection to anchor for initial transform
            Matrix4d initial_transform = findInitialTransformRelativeToAnchor(
                piece_id, anchor_piece_id, all_connections);

            cluster.transformations.push_back(initial_transform);

            if (config_.enable_detailed_logging) {
                Vector3d position = initial_transform.block<3,1>(0,3);
                std::cout << "*** RELATIVE TRANSFORM *** Piece " << piece_id
                          << " positioned at [" << std::fixed << std::setprecision(1)
                          << position[0] << ", " << position[1] << ", " << position[2]
                          << "] relative to anchor" << std::endl;
            }
        }

        // Initialize individual piece scores (will be populated during connection analysis)
        cluster.individual_piece_scores.push_back(0.0);

        // Initialize cluster properties
        cluster.centroid = Vector3d::Zero();  // Will be updated during spatial analysis
        cluster.spatial_extent = 0.0;
        cluster.cluster_quality_score = 10.0;  // Base quality for single-piece clusters

        // Internal connections start empty (single piece clusters have no internal connections)
        cluster.internal_connections.clear();

        clusters.push_back(cluster);
    }

    std::cout << "*** INITIALIZATION COMPLETE *** Created " << clusters.size()
              << " clusters with anchor-based coordinate system" << std::endl;

    return clusters;
}

int AutoAgglomerativeAssembler::selectAnchorFragment(const std::vector<LCSIndex>& all_connections, int total_pieces) {
    // ANCHOR SELECTION STRATEGY (PuzzleFusion++ Algorithm)
    // Select fragment with highest connectivity as anchor to minimize transformation ambiguity

    std::vector<int> connectivity_count(total_pieces + 1, 0);  // 1-indexed

    // Count connections for each piece
    for (const auto& connection : all_connections) {
        if (connection.shard_x_ >= 1 && connection.shard_x_ <= total_pieces) {
            connectivity_count[connection.shard_x_]++;
        }
        if (connection.shard_y_ >= 1 && connection.shard_y_ <= total_pieces) {
            connectivity_count[connection.shard_y_]++;
        }
    }

    // Find piece with maximum connectivity
    int anchor_piece = 1;
    int max_connectivity = connectivity_count[1];

    for (int piece_id = 2; piece_id <= total_pieces; ++piece_id) {
        if (connectivity_count[piece_id] > max_connectivity) {
            max_connectivity = connectivity_count[piece_id];
            anchor_piece = piece_id;
        }
    }

    if (config_.enable_detailed_logging) {
        std::cout << "*** ANCHOR ANALYSIS *** Connectivity counts:" << std::endl;
        for (int piece_id = 1; piece_id <= total_pieces; ++piece_id) {
            std::cout << "  Piece " << piece_id << ": " << connectivity_count[piece_id] << " connections";
            if (piece_id == anchor_piece) {
                std::cout << " (SELECTED AS ANCHOR)";
            }
            std::cout << std::endl;
        }
    }

    return anchor_piece;
}

Matrix4d AutoAgglomerativeAssembler::findInitialTransformRelativeToAnchor(int piece_id, int anchor_piece_id,
                                                                         const std::vector<LCSIndex>& all_connections) {
    // Find best connection between this piece and anchor piece
    Matrix4d best_transform = Matrix4d::Identity();
    double best_score = 0.0;
    bool direct_connection_found = false;

    // DIRECT CONNECTION TO ANCHOR (preferred)
    for (const auto& connection : all_connections) {
        bool is_direct_connection =
            (connection.shard_x_ == piece_id && connection.shard_y_ == anchor_piece_id) ||
            (connection.shard_x_ == anchor_piece_id && connection.shard_y_ == piece_id);

        if (is_direct_connection && connection.score_ > best_score) {
            connection.trans_.Output(best_transform);
            best_score = connection.score_;
            direct_connection_found = true;

            if (config_.enable_detailed_logging) {
                std::cout << "*** DIRECT ANCHOR CONNECTION *** Piece " << piece_id
                          << " connected to anchor " << anchor_piece_id
                          << " with score " << std::fixed << std::setprecision(3) << connection.score_ << std::endl;
            }
        }
    }

    // INDIRECT CONNECTION (if no direct connection found)
    if (!direct_connection_found) {
        // Find strongest connection involving this piece (will be optimized later)
        for (const auto& connection : all_connections) {
            if ((connection.shard_x_ == piece_id || connection.shard_y_ == piece_id) &&
                connection.score_ > best_score) {
                connection.trans_.Output(best_transform);
                best_score = connection.score_;
            }
        }

        if (config_.enable_detailed_logging) {
            std::cout << "*** INDIRECT ANCHOR CONNECTION *** Piece " << piece_id
                      << " using strongest available connection (score "
                      << std::fixed << std::setprecision(3) << best_score << ")" << std::endl;
        }
    }

    return best_transform;
}

//==================== Main Assembly Function ====================//

AutoAgglomerativeAssembler::AssemblyResult
AutoAgglomerativeAssembler::assembleGlobally(const std::vector<LCSIndex>& all_connections,
                                            std::vector<Geom>& shard,
                                            int total_pieces) {

    start_time_ = std::chrono::high_resolution_clock::now();

    std::cout << "#################### AUTO-AGGLOMERATIVE ASSEMBLY ####################" << std::endl;
    std::cout << "*** GLOBAL OPTIMIZATION *** Replacing incremental graph building" << std::endl;
    std::cout << "Input: " << all_connections.size() << " connections, " << total_pieces << " pieces" << std::endl;

    AssemblyResult result;
    result.initial_cluster_count = total_pieces;

    try {
        // Initialize: Each piece as individual cluster
        std::vector<FragmentCluster> clusters = initializeClusters(total_pieces, all_connections);

        if (!validateClusterConsistency(clusters)) {
            result.success = false;
            result.completion_status = "Initial cluster validation failed";
            return result;
        }

        std::cout << "*** INITIALIZATION *** Created " << clusters.size() << " initial clusters" << std::endl;

        // MAIN AGGLOMERATIVE LOOP
        for (int iteration = 0; iteration < config_.max_agglomerative_iterations; ++iteration) {

            if (config_.enable_detailed_logging) {
                logAssemblyProgress(clusters, iteration, "ITERATION_START");
            }

            std::cout << "*** AGGLOMERATIVE ITERATION " << (iteration + 1) << " ***" << std::endl;
            std::cout << "Current state: " << clusters.size() << " clusters" << std::endl;

            // PHASE 1: DENOISER - Refine positions within each cluster
            std::cout << "Phase 1: Position refinement..." << std::endl;
            for (auto& cluster : clusters) {
                if (cluster.size() > 1) {  // Only refine multi-piece clusters
                    refineClusterPositions(cluster, all_connections, shard);
                }
            }

            // PHASE 2: VERIFIER - Find best cluster merge candidates
            std::cout << "Phase 2: Merge candidate evaluation..." << std::endl;
            auto merge_candidates = findBestClusterMerges(clusters, all_connections, shard);

            if (merge_candidates.empty()) {
                std::cout << "*** CONVERGENCE *** No beneficial merges found" << std::endl;
                result.completion_status = "Converged - no more beneficial merges";
                break;
            }

            std::cout << "Found " << merge_candidates.size() << " merge candidates" << std::endl;

            // PHASE 3: AGGLOMERATIVE MERGE - Execute best merge
            std::cout << "Phase 3: Executing best merge..." << std::endl;
            bool merge_successful = executeMerge(clusters, merge_candidates[0], all_connections, shard);

            if (merge_successful) {
                result.total_merges_executed++;
                std::cout << "*** MERGE SUCCESS *** Clusters reduced to " << clusters.size() << std::endl;
            } else {
                std::cout << "*** MERGE FAILED *** Unable to execute merge" << std::endl;
            }

            // Update iteration statistics
            result.iterations_completed = iteration + 1;
            double current_score = 0.0;
            for (const auto& cluster : clusters) {
                current_score += cluster.cluster_quality_score;
            }
            result.iteration_scores.push_back(current_score);

            // Check for global success (single unified cluster)
            if (clusters.size() == 1) {
                std::cout << "*** GLOBAL SUCCESS *** Unified assembly achieved!" << std::endl;
                result.completion_status = "Success - unified assembly achieved";
                break;
            }

            // Validate cluster consistency
            if (!validateClusterConsistency(clusters)) {
                std::cout << "*** ERROR *** Cluster consistency validation failed" << std::endl;
                result.success = false;
                result.completion_status = "Failed - cluster consistency violation";
                return result;
            }
        }

        // Finalize result
        result.final_clusters = clusters;
        result.final_cluster_count = static_cast<int>(clusters.size());
        result.success = (clusters.size() <= 3);  // Accept up to 3 clusters as reasonable success

        // Calculate final statistics
        result.total_pieces_assembled = 0;
        result.global_assembly_score = 0.0;
        for (const auto& cluster : clusters) {
            result.total_pieces_assembled += cluster.size();
            result.global_assembly_score += cluster.cluster_quality_score;
        }

        // Calculate computation time
        auto end_time = std::chrono::high_resolution_clock::now();
        auto duration = std::chrono::duration_cast<std::chrono::milliseconds>(end_time - start_time_);
        result.computation_time_seconds = duration.count() / 1000.0;

        std::cout << "*** ASSEMBLY COMPLETE *** Final state: " << result.final_cluster_count
                  << " clusters, " << result.total_pieces_assembled << "/" << total_pieces
                  << " pieces, score=" << std::fixed << std::setprecision(3)
                  << result.global_assembly_score << std::endl;

        if (config_.enable_detailed_logging) {
            std::cout << result.generateDetailedReport() << std::endl;
        }

    } catch (const std::exception& e) {
        std::cout << "*** ASSEMBLY ERROR *** " << e.what() << std::endl;
        result.success = false;
        result.completion_status = std::string("Exception: ") + e.what();
    }

    return result;
}

//==================== Phase 1: DENOISER - Position Refinement ====================//

void AutoAgglomerativeAssembler::refineClusterPositions(FragmentCluster& cluster,
                                                       const std::vector<LCSIndex>& all_connections,
                                                       const std::vector<Geom>& shard) {

    if (cluster.size() <= 1) return;  // Single pieces don't need refinement

    if (config_.enable_detailed_logging) {
        std::cout << "*** DENOISER *** Refining positions for cluster with " << cluster.size() << " pieces" << std::endl;
    }

    double initial_quality = cluster.cluster_quality_score;

    for (int refinement = 0; refinement < config_.max_refinement_iterations; ++refinement) {
        bool any_improvement = false;

        // Refine position of each piece considering all others in cluster
        for (int i = 0; i < cluster.size(); ++i) {
            int piece_id = cluster.piece_ids[i];

            // Compute optimal position considering cluster context
            Matrix4d refined_transform = computeOptimalPositionInCluster(
                piece_id, i, cluster, all_connections, shard);

            // Test if this improves cluster quality
            Matrix4d original_transform = cluster.transformations[i];
            cluster.transformations[i] = refined_transform;

            double new_quality = evaluateClusterQuality(cluster, all_connections, shard);

            if (new_quality > cluster.cluster_quality_score + config_.quality_improvement_threshold) {
                cluster.cluster_quality_score = new_quality;
                any_improvement = true;

                if (config_.enable_detailed_logging) {
                    std::cout << "*** PIECE " << piece_id << " IMPROVED *** Quality: "
                              << std::fixed << std::setprecision(4) << new_quality << std::endl;
                }
            } else {
                // Revert if no significant improvement
                cluster.transformations[i] = original_transform;
            }
        }

        // Check for refinement convergence
        if (!any_improvement) {
            if (config_.enable_detailed_logging) {
                std::cout << "*** REFINEMENT CONVERGED *** After " << (refinement + 1) << " iterations" << std::endl;
            }
            break;
        }
    }

    // Update cluster spatial properties after refinement
    updateClusterSpatialProperties(cluster, shard);

    double improvement = cluster.cluster_quality_score - initial_quality;
    if (config_.enable_detailed_logging && improvement > 0.001) {
        std::cout << "*** DENOISER COMPLETE *** Quality improved by "
                  << std::fixed << std::setprecision(4) << improvement << std::endl;
    }
}

//==================== Phase 2: VERIFIER - Merge Candidate Evaluation ====================//

std::vector<AutoAgglomerativeAssembler::MergeCandidate>
AutoAgglomerativeAssembler::findBestClusterMerges(const std::vector<FragmentCluster>& clusters,
                                                 const std::vector<LCSIndex>& all_connections,
                                                 const std::vector<Geom>& shard) {

    std::vector<MergeCandidate> candidates;

    if (config_.enable_detailed_logging) {
        std::cout << "*** VERIFIER *** Evaluating all possible cluster merges" << std::endl;
    }

    // Evaluate all possible cluster pairs
    for (int i = 0; i < clusters.size(); ++i) {
        for (int j = i + 1; j < clusters.size(); ++j) {

            // Find connections between these clusters
            auto connecting_edges = findConnectionsBetweenClusters(clusters[i], clusters[j], all_connections);

            if (connecting_edges.empty()) continue;  // No connections = no merge possible

            MergeCandidate candidate;
            candidate.cluster1_index = i;
            candidate.cluster2_index = j;
            candidate.connecting_edge_indices = connecting_edges;

            // Evaluate merge quality
            candidate.merge_quality_score = evaluateMergeQuality(
                clusters[i], clusters[j], connecting_edges, shard);

            // Assess geometric compatibility
            candidate.geometric_compatibility = assessGeometricCompatibility(
                clusters[i], clusters[j], all_connections);

            // Calculate spatial proximity
            double distance = (clusters[i].centroid - clusters[j].centroid).norm();
            candidate.spatial_proximity = std::max(0.0, 1.0 - distance / SPATIAL_PROXIMITY_THRESHOLD);

            // Calculate connectivity strength
            candidate.connectivity_strength = 0.0;
            for (int edge_idx : connecting_edges) {
                if (edge_idx >= 0 && edge_idx < all_connections.size()) {
                    candidate.connectivity_strength += all_connections[edge_idx].score_;
                }
            }

            // Combined score calculation scaled to 0-100 range (matching original code approach)
            double scaled_merge_quality = candidate.merge_quality_score * 100.0;  // Scale initial quality to 0-100
            double scaled_connectivity = std::min(100.0, candidate.connectivity_strength);  // Cap at 100

            candidate.merge_quality_score =
                0.4 * scaled_merge_quality +
                0.3 * (candidate.geometric_compatibility * 100.0) +
                0.2 * (candidate.spatial_proximity * 100.0) +
                0.1 * scaled_connectivity;

            // PUZZLEFUSION++ PROGRESSIVE CONFIDENCE MERGING
            // Calculate confidence score (0-1 scale) for merge decision
            double merge_confidence = std::min(1.0, candidate.merge_quality_score / 100.0);

            // PuzzleFusion++ Verifier Strategy: Only merge fragments with high confidence scores (>0.9)
            const double PUZZLEFUSION_CONFIDENCE_THRESHOLD = 0.9;  // 90% confidence

            if (merge_confidence > PUZZLEFUSION_CONFIDENCE_THRESHOLD &&
                candidate.geometric_compatibility >= config_.min_geometric_compatibility) {

                candidates.push_back(candidate);

                if (config_.enable_detailed_logging) {
                    std::cout << "*** PUZZLEFUSION++ VERIFIER *** Clusters " << i << "-" << j
                              << " ACCEPTED with confidence=" << std::fixed << std::setprecision(3) << merge_confidence
                              << " (score=" << candidate.merge_quality_score
                              << ", geom=" << candidate.geometric_compatibility
                              << ", prox=" << candidate.spatial_proximity
                              << ", conn=" << candidate.connectivity_strength << ")" << std::endl;
                }
            } else if (config_.enable_detailed_logging) {
                std::cout << "*** CONFIDENCE FILTER *** Clusters " << i << "-" << j
                          << " REJECTED: confidence=" << std::fixed << std::setprecision(3) << merge_confidence
                          << " < threshold=" << PUZZLEFUSION_CONFIDENCE_THRESHOLD << std::endl;
            }
        }
    }

    // Sort by merge quality (best first)
    std::sort(candidates.begin(), candidates.end(),
              [](const MergeCandidate& a, const MergeCandidate& b) {
                  return a.merge_quality_score > b.merge_quality_score;
              });

    if (config_.enable_detailed_logging) {
        std::cout << "*** VERIFIER COMPLETE *** Found " << candidates.size()
                  << " viable merge candidates" << std::endl;
    }

    return candidates;
}

//==================== Phase 3: AGGLOMERATIVE MERGE ====================//

bool AutoAgglomerativeAssembler::executeMerge(std::vector<FragmentCluster>& clusters,
                                             const MergeCandidate& merge_candidate,
                                             const std::vector<LCSIndex>& all_connections,
                                             const std::vector<Geom>& shard) {

    if (!merge_candidate.isValid() ||
        merge_candidate.cluster1_index >= clusters.size() ||
        merge_candidate.cluster2_index >= clusters.size()) {
        return false;
    }

    if (config_.enable_detailed_logging) {
        std::cout << "*** AGGLOMERATIVE MERGE *** Merging clusters "
                  << merge_candidate.cluster1_index << " and " << merge_candidate.cluster2_index << std::endl;
    }

    int idx1 = merge_candidate.cluster1_index;
    int idx2 = merge_candidate.cluster2_index;

    // Ensure we merge smaller into larger (preserve larger cluster structure)
    if (clusters[idx1].size() < clusters[idx2].size()) {
        std::swap(idx1, idx2);
    }

    auto& target_cluster = clusters[idx1];   // Larger cluster (target)
    auto& source_cluster = clusters[idx2];   // Smaller cluster (source)

    if (config_.enable_detailed_logging) {
        std::cout << "*** MERGE DIRECTION *** Merging " << source_cluster.size()
                  << " pieces INTO " << target_cluster.size() << " pieces" << std::endl;
    }

    try {
        // Store original state for rollback if needed
        FragmentCluster original_target = target_cluster;

        // STEP 1: Merge piece lists and data
        target_cluster.piece_ids.insert(target_cluster.piece_ids.end(),
                                       source_cluster.piece_ids.begin(),
                                       source_cluster.piece_ids.end());

        target_cluster.individual_piece_scores.insert(target_cluster.individual_piece_scores.end(),
                                                     source_cluster.individual_piece_scores.begin(),
                                                     source_cluster.individual_piece_scores.end());

        // STEP 2: SPATIAL TRANSFORMATION OPTIMIZATION FOR MERGE
        // Apply proper spatial alignment between clusters before concatenating transformations

        // Find the strongest connecting edge between clusters to use as alignment reference
        Matrix4d optimal_alignment = Matrix4d::Identity();
        double best_connection_score = 0.0;
        bool alignment_found = false;

        for (int edge_idx : merge_candidate.connecting_edge_indices) {
            if (edge_idx >= 0 && edge_idx < all_connections.size()) {
                const auto& connection = all_connections[edge_idx];

                if (connection.score_ > best_connection_score) {
                    Matrix4d connection_transform;
                    connection.trans_.Output(connection_transform);

                    // Determine if this connection suggests source cluster alignment
                    bool source_needs_alignment = false;
                    for (int source_piece : source_cluster.piece_ids) {
                        if (connection.shard_x_ == source_piece || connection.shard_y_ == source_piece) {
                            source_needs_alignment = true;
                            break;
                        }
                    }

                    if (source_needs_alignment) {
                        optimal_alignment = connection_transform;
                        best_connection_score = connection.score_;
                        alignment_found = true;
                    }
                }
            }
        }

        // STEP 3: Apply spatial alignment transformation to source cluster pieces
        std::vector<Matrix4d> aligned_source_transformations;
        aligned_source_transformations.reserve(source_cluster.transformations.size());

        if (alignment_found && config_.enable_detailed_logging) {
            std::cout << "*** SPATIAL ALIGNMENT *** Applying optimal alignment with score "
                      << std::fixed << std::setprecision(3) << best_connection_score << std::endl;
        }

        for (int i = 0; i < source_cluster.transformations.size(); ++i) {
            Matrix4d source_transform = source_cluster.transformations[i];

            if (alignment_found) {
                // SPATIAL TRANSFORMATION APPLICATION:
                // Apply the optimal alignment transformation to properly position
                // source cluster pieces relative to target cluster coordinate system
                Matrix4d aligned_transform = optimal_alignment * source_transform;

                // Additional spatial optimization: adjust for better connectivity
                // Extract translation component and optimize spatial proximity
                Vector3d source_position = aligned_transform.block<3,1>(0,3);

                // Find closest target cluster piece and adjust for proximity
                double min_distance_to_target = std::numeric_limits<double>::max();
                Vector3d closest_target_position = Vector3d::Zero();

                for (const auto& target_transform : target_cluster.transformations) {
                    Vector3d target_position = target_transform.block<3,1>(0,3);
                    double distance = (source_position - target_position).norm();

                    if (distance < min_distance_to_target) {
                        min_distance_to_target = distance;
                        closest_target_position = target_position;
                    }
                }

                // Apply proximity-based optimization if clusters are too far apart
                const double max_reasonable_distance = 20.0;  // mm
                if (min_distance_to_target > max_reasonable_distance) {
                    // Adjust position to be closer to target cluster
                    Vector3d direction = (closest_target_position - source_position).normalized();
                    Vector3d optimized_position = source_position + direction * (min_distance_to_target - max_reasonable_distance) * 0.5;

                    aligned_transform.block<3,1>(0,3) = optimized_position;

                    if (config_.enable_detailed_logging) {
                        std::cout << "*** PROXIMITY OPTIMIZATION *** Adjusted piece " << source_cluster.piece_ids[i]
                                  << " distance: " << min_distance_to_target << " -> "
                                  << (optimized_position - closest_target_position).norm() << " mm" << std::endl;
                    }
                }

                aligned_source_transformations.push_back(aligned_transform);
            } else {
                // No optimal alignment found, use original transformation
                aligned_source_transformations.push_back(source_transform);
            }
        }

        // STEP 4: Merge aligned transformations
        target_cluster.transformations.insert(target_cluster.transformations.end(),
                                             aligned_source_transformations.begin(),
                                             aligned_source_transformations.end());

        // Merge connection lists
        target_cluster.internal_connections.insert(target_cluster.internal_connections.end(),
                                                  source_cluster.internal_connections.begin(),
                                                  source_cluster.internal_connections.end());

        // Add connecting edges to internal connections
        target_cluster.internal_connections.insert(target_cluster.internal_connections.end(),
                                                  merge_candidate.connecting_edge_indices.begin(),
                                                  merge_candidate.connecting_edge_indices.end());

        // Update cluster properties
        updateClusterSpatialProperties(target_cluster, shard);
        target_cluster.cluster_quality_score = evaluateClusterQuality(target_cluster, all_connections, shard);

        // Validate merged cluster
        if (!target_cluster.isValid() ||
            target_cluster.spatial_extent > config_.max_spatial_extent_ratio * SPATIAL_PROXIMITY_THRESHOLD) {

            // Rollback on validation failure
            target_cluster = original_target;
            return false;
        }

        // Remove source cluster (merged into target)
        clusters.erase(clusters.begin() + idx2);

        if (config_.enable_detailed_logging) {
            std::cout << "*** MERGE SUCCESS *** New cluster has " << target_cluster.size()
                      << " pieces, quality=" << std::fixed << std::setprecision(3)
                      << target_cluster.cluster_quality_score << std::endl;
        }

        return true;

    } catch (const std::exception& e) {
        std::cout << "*** MERGE ERROR *** " << e.what() << std::endl;
        return false;
    }
}

//==================== Quality Assessment Functions ====================//

double AutoAgglomerativeAssembler::evaluateClusterQuality(const FragmentCluster& cluster,
                                                         const std::vector<LCSIndex>& all_connections,
                                                         const std::vector<Geom>& shard) const {

    total_quality_evaluations_++;

    // Generate cache key for performance optimization
    std::stringstream cache_key_stream;
    for (int piece_id : cluster.piece_ids) {
        cache_key_stream << piece_id << ",";
    }
    std::string cache_key = cache_key_stream.str();

    if (config_.enable_quality_caching && quality_cache_.find(cache_key) != quality_cache_.end()) {
        cache_hits_++;
        return quality_cache_[cache_key];
    }

    cache_misses_++;

    if (cluster.empty()) return 0.0;

    // SPATIAL ALIGNMENT QUALITY ASSESSMENT
    // Evaluate how well 6-DoF transformations achieve spatial connectivity

    double spatial_alignment_score = 0.0;
    double connection_score_sum = 0.0;
    double proximity_score_sum = 0.0;
    int valid_connections = 0;

    // Evaluate internal connections within cluster
    for (int edge_idx : cluster.internal_connections) {
        if (edge_idx >= 0 && edge_idx < all_connections.size()) {
            const auto& connection = all_connections[edge_idx];

            // Verify both pieces are in this cluster
            bool piece1_in_cluster = std::find(cluster.piece_ids.begin(), cluster.piece_ids.end(),
                                              connection.shard_x_) != cluster.piece_ids.end();
            bool piece2_in_cluster = std::find(cluster.piece_ids.begin(), cluster.piece_ids.end(),
                                              connection.shard_y_) != cluster.piece_ids.end();

            if (piece1_in_cluster && piece2_in_cluster) {

                // STEP 1: Connection strength score
                connection_score_sum += connection.score_;

                // STEP 2: Spatial proximity evaluation after transformation
                // Find transformation indices for these pieces
                int piece1_idx = -1, piece2_idx = -1;
                for (int i = 0; i < cluster.piece_ids.size(); ++i) {
                    if (cluster.piece_ids[i] == connection.shard_x_) piece1_idx = i;
                    if (cluster.piece_ids[i] == connection.shard_y_) piece2_idx = i;
                }

                if (piece1_idx >= 0 && piece2_idx >= 0 &&
                    piece1_idx < cluster.transformations.size() &&
                    piece2_idx < cluster.transformations.size()) {

                    // Extract 6-DoF parameters from current transformations
                    Matrix4d transform1 = cluster.transformations[piece1_idx];
                    Matrix4d transform2 = cluster.transformations[piece2_idx];

                    Vector3d position1 = transform1.block<3,1>(0,3);
                    Vector3d position2 = transform2.block<3,1>(0,3);

                    // SPATIAL PROXIMITY ASSESSMENT
                    double spatial_distance = (position1 - position2).norm();

                    // Proximity score (closer = better, up to contact threshold)
                    const double contact_threshold = 5.0;  // mm
                    double proximity_score = 0.0;

                    if (spatial_distance <= contact_threshold) {
                        // Excellent - pieces are in contact
                        proximity_score = 100.0 * (1.0 - spatial_distance / contact_threshold);
                    } else {
                        // Penalize distance beyond contact threshold
                        proximity_score = std::max(0.0, 50.0 - spatial_distance);
                    }

                    proximity_score_sum += proximity_score;

                    // STEP 3: Rotation alignment assessment
                    Matrix3d rotation1 = transform1.block<3,3>(0,0);
                    Matrix3d rotation2 = transform2.block<3,3>(0,0);

                    // Measure rotation similarity (closer to identity = better alignment)
                    Matrix3d rotation_diff = rotation1.transpose() * rotation2;
                    double rotation_trace = rotation_diff.trace();
                    double rotation_alignment = (rotation_trace - 1.0) / 2.0;  // Range [-1, 1]
                    rotation_alignment = std::max(0.0, rotation_alignment);  // Clamp to [0, 1]

                    // STEP 4: Combined spatial alignment score
                    double alignment_score = 0.6 * proximity_score +
                                           0.3 * connection.score_ +
                                           0.1 * (rotation_alignment * 100.0);

                    spatial_alignment_score += alignment_score;
                }

                valid_connections++;
            }
        }
    }

    if (valid_connections == 0) {
        // Single piece cluster - use modest base quality
        double final_quality = 10.0;
        if (config_.enable_quality_caching) {
            quality_cache_[cache_key] = final_quality;
        }
        return final_quality;
    }

    // STEP 5: Normalize and combine scores
    double avg_spatial_alignment = spatial_alignment_score / valid_connections;
    double avg_connection_score = connection_score_sum / valid_connections;
    double avg_proximity_score = proximity_score_sum / valid_connections;

    // STEP 6: Compactness bonus (reward spatially coherent clusters)
    double compactness_bonus = 1.0;
    if (cluster.spatial_extent > 0) {
        // Bonus for compact clusters, penalty for scattered clusters
        compactness_bonus = 1.0 / (1.0 + cluster.spatial_extent / SPATIAL_PROXIMITY_THRESHOLD);
    }

    // STEP 7: Final quality calculation
    // High weight on spatial alignment (key to 6-DoF optimization success)
    double final_quality = (0.7 * avg_spatial_alignment +
                           0.2 * avg_connection_score +
                           0.1 * avg_proximity_score) * compactness_bonus;

    // Ensure positive quality scores
    final_quality = std::max(0.1, final_quality);

    if (config_.enable_detailed_logging && valid_connections > 0) {
        std::cout << "*** CLUSTER QUALITY *** " << cluster.size() << " pieces: "
                  << "spatial=" << std::fixed << std::setprecision(2) << avg_spatial_alignment
                  << " conn=" << avg_connection_score
                  << " prox=" << avg_proximity_score
                  << " final=" << final_quality << std::endl;
    }

    // Cache result for performance
    if (config_.enable_quality_caching) {
        quality_cache_[cache_key] = final_quality;
    }

    return final_quality;
}

double AutoAgglomerativeAssembler::evaluateMergeQuality(const FragmentCluster& cluster1,
                                                       const FragmentCluster& cluster2,
                                                       const std::vector<int>& connecting_edges,
                                                       const std::vector<Geom>& shard) const {

    if (connecting_edges.empty()) return 0.0;

    // Create hypothetical merged cluster for evaluation
    FragmentCluster hypothetical_cluster;
    hypothetical_cluster.piece_ids = cluster1.piece_ids;
    hypothetical_cluster.piece_ids.insert(hypothetical_cluster.piece_ids.end(),
                                         cluster2.piece_ids.begin(),
                                         cluster2.piece_ids.end());

    hypothetical_cluster.internal_connections = cluster1.internal_connections;
    hypothetical_cluster.internal_connections.insert(hypothetical_cluster.internal_connections.end(),
                                                    cluster2.internal_connections.begin(),
                                                    cluster2.internal_connections.end());
    hypothetical_cluster.internal_connections.insert(hypothetical_cluster.internal_connections.end(),
                                                    connecting_edges.begin(),
                                                    connecting_edges.end());

    // Temporarily compute spatial properties
    Vector3d combined_centroid = (cluster1.centroid * cluster1.size() + cluster2.centroid * cluster2.size())
                                / (cluster1.size() + cluster2.size());
    hypothetical_cluster.centroid = combined_centroid;

    double max_distance = 0.0;
    // Note: For full implementation, would need to iterate through all pieces and calculate max distance
    // For now, use conservative estimate
    double estimated_extent = std::max(cluster1.spatial_extent, cluster2.spatial_extent) +
                             (cluster1.centroid - cluster2.centroid).norm();
    hypothetical_cluster.spatial_extent = estimated_extent;

    return evaluateClusterQuality(hypothetical_cluster, {}, shard);  // Empty connections vector as we use internal_connections
}

//==================== Spatial Analysis Functions ====================//

Matrix4d AutoAgglomerativeAssembler::computeOptimalPositionInCluster(int piece_id,
                                                                     int piece_index_in_cluster,
                                                                     const FragmentCluster& cluster,
                                                                     const std::vector<LCSIndex>& all_connections,
                                                                     const std::vector<Geom>& shard) const {

    // 6-DoF PARAMETER DIFFUSION DENOISING IMPLEMENTATION
    // PuzzleFusion++ inspired iterative refinement of transformation parameters

    Matrix4d current_transform = cluster.transformations[piece_index_in_cluster];

    // Find connections involving this piece within the cluster
    std::vector<LCSIndex> relevant_connections;
    for (int edge_idx : cluster.internal_connections) {
        if (edge_idx >= 0 && edge_idx < all_connections.size()) {
            const auto& connection = all_connections[edge_idx];
            if (connection.shard_x_ == piece_id || connection.shard_y_ == piece_id) {
                relevant_connections.push_back(connection);
            }
        }
    }

    if (relevant_connections.empty()) {
        return current_transform;  // No connections to optimize against
    }

    // STEP 1: Initialize with strongest connection as "noisy" parameters
    auto best_connection = *std::max_element(relevant_connections.begin(), relevant_connections.end(),
        [](const LCSIndex& a, const LCSIndex& b) {
            return a.score_ < b.score_;
        });

    Matrix4d base_transform;
    best_connection.trans_.Output(base_transform);

    // STEP 2: ENHANCED SE3 DIFFUSION DENOISING PROCESS
    // PuzzleFusion++ Algorithm: Custom noise scheduler that dedicates more denoising steps to precise alignments
    Matrix4d optimized_transform = base_transform;
    double best_total_score = 0.0;

    // ADAPTIVE NOISE SCHEDULING (PuzzleFusion++ Innovation)
    // Allocate more denoising steps to fine-tuning local alignments
    const int coarse_iterations = 3;      // Quickly identify rough fragment locations
    const int fine_iterations = 12;       // Spend computational budget on precise geometric matching
    const int max_denoising_iterations = coarse_iterations + fine_iterations;
    const double coarse_convergence_threshold = 0.01;   // Loose threshold for coarse alignment
    const double fine_convergence_threshold = 0.0001;   // Tight threshold for precise alignment

    for (int iteration = 0; iteration < max_denoising_iterations; ++iteration) {

        // STEP 3: GLOBAL CONTEXT OPTIMIZATION
        // Consider ALL relevant connections for this piece to optimize 6-DoF parameters
        double total_alignment_score = 0.0;
        Vector3d translation_gradient = Vector3d::Zero();
        Vector3d rotation_gradient = Vector3d::Zero();
        int active_connections = 0;

        for (const auto& connection : relevant_connections) {
            Matrix4d connection_transform;
            connection.trans_.Output(connection_transform);

            // Extract current 6-DoF parameters
            Vector3d current_translation = optimized_transform.block<3,1>(0,3);
            Matrix3d current_rotation = optimized_transform.block<3,3>(0,0);

            // Extract target 6-DoF parameters from connection
            Vector3d target_translation = connection_transform.block<3,1>(0,3);
            Matrix3d target_rotation = connection_transform.block<3,3>(0,0);

            // Compute 6-DoF parameter differences (denoising gradients)
            Vector3d translation_diff = target_translation - current_translation;

            // Rotation difference using angle-axis representation
            Matrix3d rotation_diff = target_rotation * current_rotation.transpose();
            Eigen::AngleAxisd angle_axis(rotation_diff);
            Vector3d rotation_diff_vec = angle_axis.angle() * angle_axis.axis();

            // Weight by connection strength (mimics diffusion model confidence)
            double connection_weight = connection.score_ / 100.0;  // Normalize score

            translation_gradient += connection_weight * translation_diff;
            rotation_gradient += connection_weight * rotation_diff_vec;

            total_alignment_score += connection_weight * connection.score_;
            active_connections++;
        }

        if (active_connections == 0) break;

        // STEP 4: ADAPTIVE PARAMETER UPDATE (PuzzleFusion++ Noise Scheduling)
        // Different learning rates and thresholds for coarse vs fine phases
        bool is_coarse_phase = (iteration < coarse_iterations);
        double learning_rate = is_coarse_phase ? 0.5 : 0.1;  // Aggressive then conservative
        double current_threshold = is_coarse_phase ? coarse_convergence_threshold : fine_convergence_threshold;

        // Average gradients
        translation_gradient /= active_connections;
        rotation_gradient /= active_connections;

        // ENHANCED SE3 DIFFUSION DENOISER
        // Apply different optimization strategies for coarse vs fine phases
        Vector3d new_translation;
        Matrix3d new_rotation;

        if (is_coarse_phase) {
            // COARSE PHASE: Quickly identify rough fragment locations
            new_translation = optimized_transform.block<3,1>(0,3) + learning_rate * translation_gradient;

            // More aggressive rotation updates for coarse alignment
            Eigen::AngleAxisd rotation_update(learning_rate * rotation_gradient.norm(),
                                             rotation_gradient.normalized());
            new_rotation = rotation_update.toRotationMatrix() * optimized_transform.block<3,3>(0,0);
        } else {
            // FINE PHASE: Precise geometric matching with smaller steps
            // Apply dampening for stability in fine-tuning
            Vector3d dampened_translation = translation_gradient * 0.5;  // Reduce noise
            Vector3d dampened_rotation = rotation_gradient * 0.5;

            new_translation = optimized_transform.block<3,1>(0,3) + learning_rate * dampened_translation;

            // Conservative rotation updates for precise alignment
            Eigen::AngleAxisd rotation_update(learning_rate * dampened_rotation.norm(),
                                             dampened_rotation.normalized());
            new_rotation = rotation_update.toRotationMatrix() * optimized_transform.block<3,3>(0,0);
        }

        // Construct updated transformation matrix
        Matrix4d candidate_transform = Matrix4d::Identity();
        candidate_transform.block<3,3>(0,0) = new_rotation;
        candidate_transform.block<3,1>(0,3) = new_translation;

        // STEP 5: ADAPTIVE CONVERGENCE CHECK
        double improvement = total_alignment_score - best_total_score;

        if (improvement > current_threshold) {
            optimized_transform = candidate_transform;
            best_total_score = total_alignment_score;

            if (config_.enable_detailed_logging) {
                std::string phase = is_coarse_phase ? "COARSE" : "FINE";
                std::cout << "*** " << phase << " DENOISING *** Piece " << piece_id
                          << " iteration " << (iteration + 1)
                          << " score=" << std::fixed << std::setprecision(3) << total_alignment_score << std::endl;
            }
        } else {
            // Check for phase transition
            if (is_coarse_phase && iteration == coarse_iterations - 1) {
                if (config_.enable_detailed_logging) {
                    std::cout << "*** PHASE TRANSITION *** Piece " << piece_id
                              << " switching to FINE alignment phase" << std::endl;
                }
            } else if (!is_coarse_phase) {
                // Converged in fine phase - optimal 6-DoF parameters achieved
                if (config_.enable_detailed_logging) {
                    std::cout << "*** SE3 CONVERGED *** Piece " << piece_id
                              << " after " << (iteration + 1) << " denoising steps" << std::endl;
                }
                break;
            }
        }
    }

    return optimized_transform;
}

std::vector<int> AutoAgglomerativeAssembler::findConnectionsBetweenClusters(
    const FragmentCluster& cluster1,
    const FragmentCluster& cluster2,
    const std::vector<LCSIndex>& all_connections) const {

    std::vector<int> connecting_edges;

    for (int i = 0; i < all_connections.size(); ++i) {
        const auto& connection = all_connections[i];

        bool piece1_in_cluster1 = std::find(cluster1.piece_ids.begin(), cluster1.piece_ids.end(),
                                           connection.shard_x_) != cluster1.piece_ids.end();
        bool piece2_in_cluster1 = std::find(cluster1.piece_ids.begin(), cluster1.piece_ids.end(),
                                           connection.shard_y_) != cluster1.piece_ids.end();
        bool piece1_in_cluster2 = std::find(cluster2.piece_ids.begin(), cluster2.piece_ids.end(),
                                           connection.shard_x_) != cluster2.piece_ids.end();
        bool piece2_in_cluster2 = std::find(cluster2.piece_ids.begin(), cluster2.piece_ids.end(),
                                           connection.shard_y_) != cluster2.piece_ids.end();

        // Connection bridges clusters if one piece is in each cluster
        if ((piece1_in_cluster1 && piece2_in_cluster2) || (piece1_in_cluster2 && piece2_in_cluster1)) {
            connecting_edges.push_back(i);
        }
    }

    return connecting_edges;
}

void AutoAgglomerativeAssembler::updateClusterSpatialProperties(FragmentCluster& cluster,
                                                               const std::vector<Geom>& shard) const {

    if (cluster.empty()) return;

    // Calculate centroid (simplified - would need actual geometry centroids in production)
    cluster.centroid = Vector3d::Zero();

    // Calculate spatial extent (simplified approach)
    cluster.spatial_extent = 0.0;

    // For production implementation, would iterate through actual geometry and calculate proper spatial properties
    // This simplified version provides basic functionality
}

//==================== Utility Functions ====================//

bool AutoAgglomerativeAssembler::validateClusterConsistency(const std::vector<FragmentCluster>& clusters) const {

    std::unordered_set<int> all_pieces;

    for (const auto& cluster : clusters) {
        if (!cluster.isValid()) {
            return false;
        }

        // Check for duplicate pieces across clusters
        for (int piece_id : cluster.piece_ids) {
            if (all_pieces.find(piece_id) != all_pieces.end()) {
                return false;  // Piece appears in multiple clusters
            }
            all_pieces.insert(piece_id);
        }
    }

    return true;
}

void AutoAgglomerativeAssembler::logAssemblyProgress(const std::vector<FragmentCluster>& clusters,
                                                    int iteration,
                                                    const std::string& phase_name) const {

    std::cout << "*** " << phase_name << " *** Iteration " << iteration
              << ", Clusters: " << clusters.size() << std::endl;

    for (int i = 0; i < clusters.size(); ++i) {
        const auto& cluster = clusters[i];
        std::cout << "  Cluster " << i << ": " << cluster.size() << " pieces, quality="
                  << std::fixed << std::setprecision(3) << cluster.cluster_quality_score << std::endl;
    }
}

//==================== Result Conversion Functions ====================//

State AutoAgglomerativeAssembler::AssemblyResult::convertToState() const {

    if (!success || final_clusters.empty()) {
        return State(0);  // Empty state for failed assembly
    }

    State global_state(total_pieces_assembled);

    // Convert each cluster to a graph
    for (const auto& cluster : final_clusters) {
        RankingSubgraph graph(total_pieces_assembled);

        // Set up node array
        graph.node_.resize(total_pieces_assembled, false);
        graph.T_.resize(total_pieces_assembled);

        for (int i = 0; i < cluster.piece_ids.size(); ++i) {
            int piece_id = cluster.piece_ids[i];
            int piece_index = piece_id - 1;  // Convert to 0-based

            if (piece_index >= 0 && piece_index < total_pieces_assembled) {
                graph.node_[piece_index] = true;
                graph.T_[piece_index].Input(cluster.transformations[i]);
            }
        }

        graph.graph_score_ = cluster.cluster_quality_score;
        global_state.graph_.push_back(graph);
    }

    global_state.state_score_ = global_assembly_score;
    global_state.SynchronizeTrueNode();

    return global_state;
}

std::string AutoAgglomerativeAssembler::AssemblyResult::generateDetailedReport() const {

    std::stringstream report;

    report << "==================== AUTO-AGGLOMERATIVE ASSEMBLY REPORT ====================" << std::endl;
    report << "Success: " << (success ? "YES" : "NO") << std::endl;
    report << "Status: " << completion_status << std::endl;
    report << "Computation Time: " << std::fixed << std::setprecision(2) << computation_time_seconds << " seconds" << std::endl;
    report << std::endl;

    report << "CLUSTER STATISTICS:" << std::endl;
    report << "  Initial clusters: " << initial_cluster_count << std::endl;
    report << "  Final clusters: " << final_cluster_count << std::endl;
    report << "  Merges executed: " << total_merges_executed << std::endl;
    report << "  Iterations completed: " << iterations_completed << std::endl;
    report << std::endl;

    report << "ASSEMBLY QUALITY:" << std::endl;
    report << "  Global score: " << std::fixed << std::setprecision(4) << global_assembly_score << std::endl;
    report << "  Pieces assembled: " << total_pieces_assembled << std::endl;
    report << std::endl;

    if (!final_clusters.empty()) {
        report << "FINAL CLUSTER DETAILS:" << std::endl;
        for (int i = 0; i < final_clusters.size(); ++i) {
            const auto& cluster = final_clusters[i];
            report << "  Cluster " << i << ": " << cluster.size() << " pieces, quality="
                   << std::fixed << std::setprecision(4) << cluster.cluster_quality_score << std::endl;
        }
    }

    report << "=============================================================================" << std::endl;

    return report.str();
}

//==================== Geometric Compatibility Assessment ====================//

double AutoAgglomerativeAssembler::assessGeometricCompatibility(const FragmentCluster& cluster1,
                                                               const FragmentCluster& cluster2,
                                                               const std::vector<LCSIndex>& connections) const {

    // Simplified geometric compatibility assessment
    // In production, this would involve detailed geometric analysis

    double compatibility = 0.5;  // Base compatibility

    // Factor in spatial proximity
    double distance = (cluster1.centroid - cluster2.centroid).norm();
    if (distance < SPATIAL_PROXIMITY_THRESHOLD) {
        compatibility += 0.3 * (1.0 - distance / SPATIAL_PROXIMITY_THRESHOLD);
    }

    // Factor in size compatibility (similar-sized clusters merge better)
    double size_ratio = static_cast<double>(std::min(cluster1.size(), cluster2.size())) /
                       std::max(cluster1.size(), cluster2.size());
    compatibility += 0.2 * size_ratio;

    return std::min(1.0, compatibility);
}