/******************************************************************************
 * graph_extractor.cpp 
 *
 * Source of DrawIT 
 *
 ******************************************************************************
 * Copyright (C) 2014 Christian Schulz <christian.schulz@kit.edu>
 *
 * This program is free software: you can redistribute it and/or modify it
 * under the terms of the GNU General Public License as published by the Free
 * Software Foundation, either version 3 of the License, or (at your option)
 * any later version.
 *
 * This program is distributed in the hope that it will be useful, but WITHOUT
 * ANY WARRANTY; without even the implied warranty of MERCHANTABILITY or
 * FITNESS FOR A PARTICULAR PURPOSE.  See the GNU General Public License for
 * more details.
 *
 * You should have received a copy of the GNU General Public License along with
 * this program.  If not, see <http://www.gnu.org/licenses/>.
 *****************************************************************************/

#include <unordered_map>
#include "graph_extractor.h"
#include "data_structure/union_find.h"


graph_extractor::graph_extractor() {

}

graph_extractor::~graph_extractor() {

}

void graph_extractor::extract_largest_component(graph_access & G, 
                                                graph_access & Q) {
     
        union_find uf(G.number_of_nodes());
        forall_nodes(G, node) {
                forall_out_edges(G, e, node) {
                        NodeID target = G.getEdgeTarget(e);
                        uf.Union(node,target);
                } endfor
        } endfor

        std::unordered_map<NodeID, NodeID> comp_size;
        forall_nodes(G, node) {
                comp_size[uf.Find(node)] += 1;
                G.setPartitionIndex(node, uf.Find(node));
        } endfor
        
        NodeID max_size = 0;
        NodeID max_key  = 0;
        for( auto it : comp_size ) {
                //std::cout <<  it.first  << std::endl;
                //std::cout <<  it.second << std::endl;

                if( it.second > max_size ) {
                        max_size = it.second;
                        max_key = it.first;
                }
        }

        std::vector< NodeID > mapping(G.number_of_nodes(), 0);
        extract_block(G, Q, max_key, mapping);
}


void graph_extractor::extract_block(graph_access & G, 
                                    graph_access & extracted_block, 
                                    PartitionID block, 
                                    std::vector<NodeID> & mapping) {

        // build reverse mapping
        std::vector<NodeID> reverse_mapping;
        NodeID nodes = 0;
        NodeID dummy_node = G.number_of_nodes() + 1;
        forall_nodes(G, node) {
                if(G.getPartitionIndex(node) == block) {
                        reverse_mapping.push_back(nodes++);
                } else {
                        reverse_mapping.push_back(dummy_node);
                }
        } endfor

        extracted_block.start_construction(nodes, G.number_of_edges());

        forall_nodes(G, node) {
                if(G.getPartitionIndex(node) == block) {
                        NodeID new_node = extracted_block.new_node();
                        mapping.push_back(node);
                        extracted_block.setNodeWeight( new_node, G.getNodeWeight(node));
                        extracted_block.setCoords( new_node, G.getX(node), G.getY(node));

                        forall_out_edges(G, e, node) {
                                NodeID target = G.getEdgeTarget(e);
                                if( G.getPartitionIndex( target ) == block ) {
                                        EdgeID new_edge = extracted_block.new_edge(new_node, reverse_mapping[target]);
                                        extracted_block.setEdgeWeight(new_edge, G.getEdgeWeight(e));
                                }
                        } endfor
                }
        } endfor

        extracted_block.finish_construction();
}

void graph_extractor::extract_all_blocks(graph_access & G, 
                std::vector< graph_access > & subgraphs,
                std::vector< std::vector<NodeID> > & mapping) {

        std::vector< NodeID > edge_count(G.get_partition_count(),0);
        std::vector< NodeID > node_count(G.get_partition_count(),0);
        std::vector< NodeID > reverse_mapping;
        mapping.resize(G.get_partition_count());
        subgraphs.resize(G.get_partition_count());

        forall_nodes(G, node) {
                NodeID cur_no_nodes_of_block = node_count[G.getPartitionIndex(node)];
                reverse_mapping.push_back(cur_no_nodes_of_block);
                node_count[G.getPartitionIndex(node)]++;

                forall_out_edges(G, e, node) {
                        NodeID target = G.getEdgeTarget(e);
                        if( G.getPartitionIndex(node) == G.getPartitionIndex(target)) {
                                edge_count[G.getPartitionIndex(node)]++;
                        }
                } endfor
        } endfor
        
        for( unsigned int block = 0; block < G.get_partition_count(); block++) {
                subgraphs[block].start_construction(node_count[block], edge_count[block]);
        }

        forall_nodes(G, node) {
                PartitionID block = G.getPartitionIndex(node);
                NodeID new_node = subgraphs[block].new_node();
                mapping[block].push_back(node);
                subgraphs[block].setCoords(new_node, G.getX(node), G.getY(node));
                subgraphs[block].setNodeWeight(new_node, G.getNodeWeight(node));
                subgraphs[block].setPartitionIndex(new_node, 0);

                forall_out_edges(G, e, node) {
                        NodeID target = G.getEdgeTarget(e);
                        if( G.getPartitionIndex( target ) == block ) { // node is in the same subgraph
                                EdgeID new_edge = subgraphs[block].new_edge(new_node, reverse_mapping[target]);
                                subgraphs[block].setEdgeWeight(new_edge, G.getEdgeWeight(e));
                        }
                } endfor
        } endfor

        for( unsigned int block = 0; block < G.get_partition_count(); block++) {
                subgraphs[block].finish_construction();
        }
}

void graph_extractor::extract_two_blocks(graph_access & G, 
                                         graph_access & extracted_block_lhs, 
                                         graph_access & extracted_block_rhs, 
                                         std::vector<NodeID> & mapping_lhs,
                                         std::vector<NodeID> & mapping_rhs,
                                         NodeWeight & partition_weight_lhs,
                                         NodeWeight & partition_weight_rhs) {

        PartitionID lhs = 0;
        PartitionID rhs = 1;

        // build reverse mapping
        std::vector<NodeID> reverse_mapping_lhs;
        std::vector<NodeID> reverse_mapping_rhs;
        NodeID nodes_lhs     = 0;
        NodeID nodes_rhs     = 0;
        partition_weight_lhs = 0;
        partition_weight_rhs = 0;
        NodeID dummy_node    = G.number_of_nodes() + 1;

        forall_nodes(G, node) {
                if(G.getPartitionIndex(node) == lhs) {
                        reverse_mapping_lhs.push_back(nodes_lhs++);
                        reverse_mapping_rhs.push_back(dummy_node);
                        partition_weight_lhs += G.getNodeWeight(node);
                } else {
                        reverse_mapping_rhs.push_back(nodes_rhs++);
                        reverse_mapping_lhs.push_back(dummy_node);
                        partition_weight_rhs += G.getNodeWeight(node);
                }
        } endfor

        extracted_block_lhs.start_construction(nodes_lhs, G.number_of_edges());
        extracted_block_rhs.start_construction(nodes_rhs, G.number_of_edges());

        forall_nodes(G, node) {
                if(G.getPartitionIndex(node) == lhs) {
                        NodeID new_node = extracted_block_lhs.new_node();
                        mapping_lhs.push_back(node);
                        extracted_block_lhs.setNodeWeight(new_node, G.getNodeWeight(node));

                        forall_out_edges(G, e, node) {
                                NodeID target = G.getEdgeTarget(e);
                                if( G.getPartitionIndex( target ) == lhs) {
                                        EdgeID new_edge = extracted_block_lhs.new_edge(new_node, reverse_mapping_lhs[target]);
                                        extracted_block_lhs.setEdgeWeight( new_edge, G.getEdgeWeight(e));
                                }
                        } endfor

                } else {
                        NodeID new_node = extracted_block_rhs.new_node();
                        mapping_rhs.push_back(node);
                        extracted_block_rhs.setNodeWeight(new_node, G.getNodeWeight(node));

                        forall_out_edges(G, e, node) {
                                NodeID target = G.getEdgeTarget(e);
                                if( G.getPartitionIndex( target ) == rhs) {
                                        EdgeID new_edge = extracted_block_rhs.new_edge(new_node, reverse_mapping_rhs[target]);
                                        extracted_block_rhs.setEdgeWeight( new_edge, G.getEdgeWeight(e));
                                }
                        } endfor
                }
        } endfor

        extracted_block_lhs.finish_construction();
        extracted_block_rhs.finish_construction();
}

// Method takes a number of nodes and extracts the underlying subgraph from G
// it also assignes block informations
void graph_extractor::extract_two_blocks_connected(graph_access & G, 
                                                   std::vector<NodeID> lhs_nodes,
                                                   std::vector<NodeID> rhs_nodes,
                                                   PartitionID lhs, 
                                                   PartitionID rhs,
                                                   graph_access & pair,
                                                   std::vector<NodeID> & mapping) {
        //// build reverse mapping
        std::unordered_map<NodeID,NodeID> reverse_mapping;
        NodeID nodes = 0;
        EdgeID edges = 0; // upper bound for number of edges

        for( unsigned i = 0; i < lhs_nodes.size(); i++) {
                NodeID node           = lhs_nodes[i];
                reverse_mapping[node] = nodes;
                edges += G.getNodeDegree(lhs_nodes[i]);
                nodes++;
        }
        for( unsigned i = 0; i < rhs_nodes.size(); i++) {
                NodeID node           = rhs_nodes[i];
                reverse_mapping[node] = nodes;
                edges += G.getNodeDegree(rhs_nodes[i]);
                nodes++;
        }

        pair.start_construction(nodes, edges);

        for( unsigned i = 0; i < lhs_nodes.size(); i++) {
                NodeID node     = lhs_nodes[i];
                NodeID new_node = pair.new_node();
                mapping.push_back(node);

                pair.setNodeWeight(new_node, G.getNodeWeight(node));
                pair.setPartitionIndex(new_node, 0);

                forall_out_edges(G, e, node) {
                        NodeID target = G.getEdgeTarget(e);
                        if( G.getPartitionIndex( target ) == lhs || G.getPartitionIndex( target ) == rhs ) {
                                EdgeID new_edge = pair.new_edge(new_node, reverse_mapping[target]);
                                pair.setEdgeWeight(new_edge, G.getEdgeWeight(e));
                        }
                } endfor

        }

        for( unsigned i = 0; i < rhs_nodes.size(); i++) {
                NodeID node     = rhs_nodes[i];
                NodeID new_node = pair.new_node();
                mapping.push_back(node);

                pair.setNodeWeight(new_node, G.getNodeWeight(node));
                pair.setPartitionIndex(new_node, 1);

                forall_out_edges(G, e, node) {
                        NodeID target = G.getEdgeTarget(e);
                        if( G.getPartitionIndex( target ) == lhs || G.getPartitionIndex( target ) == rhs ) {
                                EdgeID new_edge = pair.new_edge(new_node, reverse_mapping[target]);
                                pair.setEdgeWeight(new_edge, G.getEdgeWeight(e));
                        }
                } endfor

        }

        pair.finish_construction();
}


