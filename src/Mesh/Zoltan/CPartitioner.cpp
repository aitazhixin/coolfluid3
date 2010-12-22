// Copyright (C) 2010 von Karman Institute for Fluid Dynamics, Belgium
//
// This software is distributed under the terms of the
// GNU Lesser General Public License version 3 (LGPLv3).
// See doc/lgpl.txt and doc/gpl.txt for the license text.

// coolfluid
#include "Common/CBuilder.hpp"
#include "Common/OptionT.hpp"
#include "Common/String/Conversion.hpp"
#include "Common/MPI/PE.hpp"
#include "Common/MPI/tools.hpp"
#include "Common/Log.hpp"

#include "Mesh/Zoltan/CPartitioner.hpp"

namespace CF {
namespace Mesh {
namespace Zoltan {

  using namespace Common;
  using namespace Common::String;
  
#define RANK "[" << PE::instance().rank() << "] "

////////////////////////////////////////////////////////////////////////////////

CF::Common::ComponentBuilder < CPartitioner, CMeshPartitioner, LibZoltan > zoltan_partitioner_builder;

//////////////////////////////////////////////////////////////////////////////

CPartitioner::CPartitioner ( const std::string& name ) : 
	CMeshPartitioner(name),
	m_partitioned(false)
{
  
  m_properties.add_option<OptionT <std::string> >("Graph Package","External library zoltan will use for graph partitioning","PHG")->mark_basic();
  m_properties.add_option<OptionT <Uint> >("Debug Level","Internal Zoltan debug level (0 to 10)",0);
  
	float version;
	int error_code = Zoltan_Initialize(Core::instance().argc(),Core::instance().argv(),&version);
	cf_assert_desc("Could not initialize Zoltan",error_code == ZOLTAN_OK);
	CFdebug << "Zoltan version = " << version << CFendl;
  m_zz = new ZoltanObject(PE::instance());
  cf_assert (m_zz != NULL);
}

//////////////////////////////////////////////////////////////////////////////

CPartitioner::~CPartitioner ( )
{  
	if (m_partitioned)
	{
		ZoltanObject::LB_Free_Part(&importGlobalIds, &importLocalIds, &importProcs, &importToPart);
		ZoltanObject::LB_Free_Part(&exportGlobalIds, &exportLocalIds, &exportProcs, &exportToPart);  		
	}

  delete m_zz;
}

//////////////////////////////////////////////////////////////////////////////

void CPartitioner::partition_graph()
{ 
	CFdebug.setFilterRankZero(false);

	m_partitioned = true;
  set_partitioning_params();

  m_zz->LB_Partition(changes, numGidEntries, numLidEntries,
    numImport, importGlobalIds, importLocalIds, importProcs, importToPart,
    numExport, exportGlobalIds, exportLocalIds, exportProcs, exportToPart);
	
  m_changes->reserve(numExport);
  for (Uint i=0; i<(Uint)numExport; ++i)
  {
    m_changes->insert_blindly(exportGlobalIds[i],exportToPart[i]);
  }
  
  m_changes->sort_keys();
	
	CFdebug.setFilterRankZero(true);

  
}

//////////////////////////////////////////////////////////////////////////////

void CPartitioner::set_partitioning_params()
{
	/// Zoltan general parameters
	
  m_zz->Set_Param( "DEBUG_LEVEL", Common::String::to_str(property("Debug Level").value<Uint>()));
  //  0	Quiet mode; no output unless an error or warning is produced.
  //	1	Values of all parameters set by Zoltan_Set_Param and used by Zoltan.
  //	2	Timing information for Zoltan's main routines.
  //  3	Timing information within Zoltan's algorithms (support by algorithms is optional).
  //  4	
  //  5	Trace information (enter/exit) for major Zoltan interface routines (printed by the processor specified by the DEBUG_PROCESSOR parameter).
  //  6	Trace information (enter/exit) for major Zoltan interface routines (printed by all processors).
  //  7	More detailed trace information in major Zoltan interface routines.
  //  8	List of objects to be imported to and exported from each processor. [1]
  //  9	
  // 10	Maximum debug output; may include algorithm-specific output. [1]
  // 
	// [1] Output may be serialized; that is, one processor may have to complete its output before the next processor 
	// is allowed to begin its output.  This serialization is not scalable and can significantly increase execution 
	// time on large number of processors.
	
	
	m_zz->Set_Param( "LB_METHOD", "GRAPH");
	// The load-balancing algorithm used by Zoltan is specified by this parameter. Valid values are 
	// BLOCK (for block partitioning), 
	// RANDOM (for random partitioning), 
	// RCB (for recursive coordinate bisection), 
	// RIB (for recursive inertial bisection), 
	// HSFC (for Hilbert space-filling curve partitioning), 
	// REFTREE (for refinement tree based partitioning) 
	// GRAPH (to choose from collection of methods for graphs), 
	// HYPERGRAPH (to choose from a collection of methods for hypergraphs), 
	// HIER (for hybrid hierarchical partitioning) 
	// NONE (for no load balancing).
	
	m_zz->Set_Param( "LB_APPROACH", "PARTITION");
	// The desired load balancing approach. Only LB_METHOD = HYPERGRAPH or GRAPH 
	// uses the LB_APPROACH parameter. Valid values are
	//   PARTITION (Partition "from scratch," not taking into account the current data distribution; 
	//             this option is recommended for static load balancing.)
	//   REPARTITION (Partition but take into account current data distribution to keep data migration low;
	//               this option is recommended for dynamic load balancing.)
	//   REFINE (Quickly improve the current data distribution.)
	
  m_zz->Set_Param( "NUM_GID_ENTRIES", "1");
	// The number of unsigned integers that should be used to represent a global identifier (ID). Values greater than zero are accepted.
  
	m_zz->Set_Param( "NUM_LID_ENTRIES", "0");
  // The number of unsigned integers that should be used to represent a local identifier (ID). Values greater than or equal to zero are accepted.
	
	m_zz->Set_Param( "RETURN_LISTS", "EXPORT");
	// The lists returned by calls to Zoltan_LB_Partition. Valid values are 
	// "IMPORT", to return only information about objects to be imported to a processor 
	// "EXPORT", to return only information about objects to be exported from a processor 
	// "ALL", or "IMPORT AND EXPORT" (or any string with both "IMPORT" and "EXPORT" in it) to return both import and export information 
	// "PARTS" (or "PART ASSIGNMENT" or any string with "PART" in it) to return the new process and part assignment of every local object, including those not being exported. 
	// "NONE", to return neither import nor export information
  

  m_zz->Set_Param( "NUM_GLOBAL_PARTS", to_str(property("Number of Partitions").value<Uint>()));
	// The total number of parts to be generated by a call to Zoltan_LB_Partition.
	
	
	/// Zoltan graph parameters
	
	m_zz->Set_Param( "GRAPH_PACKAGE",property("Graph Package").value<std::string>());
	// The software package to use in partitioning the graph.  
	// PHG (default)     http://www.cs.sandia.gov/Zoltan/ug_html/ug_alg_phg.html
	// ParMETIS          http://www.cs.sandia.gov/Zoltan/ug_html/ug_alg_parmetis.html
	// Scotch/PT-Scotch  http://www.cs.sandia.gov/Zoltan/ug_html/ug_alg_ptscotch.html
	
  m_zz->Set_Param( "CHECK_GRAPH", to_str(std::max(2u,property("Debug Level").value<Uint>())));
  // Level of error checking for graph input: 
	// 0 = no checking, 
	// 1 = on-processor checking, 
	// 2 = full checking. (CHECK_GRAPH==2 is very slow and should be used only during debugging).
	
	/// Zoltan Query functions 
	
  m_zz->Set_Num_Obj_Fn(&CPartitioner::query_nb_of_objects, this);
  m_zz->Set_Obj_List_Fn(&CPartitioner::query_list_of_objects, this);
  m_zz->Set_Num_Edges_Multi_Fn(&CPartitioner::query_nb_connected_objects, this);
  m_zz->Set_Edge_List_Multi_Fn(&CPartitioner::query_list_of_connected_objects, this);
}

//////////////////////////////////////////////////////////////////////////////

int CPartitioner::query_nb_of_objects(void *data, int *ierr)
{
  CMeshPartitioner& p = *(CMeshPartitioner *)data;
  *ierr = ZOLTAN_OK;
  
  return p.nb_owned_objects();
}

//////////////////////////////////////////////////////////////////////////////

void CPartitioner::query_list_of_objects(void *data, int sizeGID, int sizeLID,
                                         ZOLTAN_ID_PTR globalID, ZOLTAN_ID_PTR localID,
                                         int wgt_dim, float *obj_wgts, int *ierr)
{
  CMeshPartitioner& p = *(CMeshPartitioner *)data;
  *ierr = ZOLTAN_OK;
  
  p.list_of_owned_objects(globalID);
	
	
	// for debugging
#if 0
  std::vector<Uint> glbID(p.nb_owned_objects());
  p.list_of_owned_objects(glbID);
  
  CFdebug << RANK << "glbID =";
  boost_foreach(const Uint g, glbID)
    CFdebug << " " << g;
  CFdebug << CFendl;
#endif
	
}

//////////////////////////////////////////////////////////////////////////////

void CPartitioner::query_nb_connected_objects(void *data, int sizeGID, int sizeLID, int num_obj,
                                              ZOLTAN_ID_PTR globalID, ZOLTAN_ID_PTR localID,
                                              int *numEdges, int *ierr)
{
  CMeshPartitioner& p = *(CMeshPartitioner *)data;
  *ierr = ZOLTAN_OK;
  
  p.nb_connected_objects(numEdges);
}

//////////////////////////////////////////////////////////////////////////////

void CPartitioner::query_list_of_connected_objects(void *data, int sizeGID, int sizeLID, int num_obj, 
                                                   ZOLTAN_ID_PTR globalID, ZOLTAN_ID_PTR localID,
                                                   int *num_edges,
                                                   ZOLTAN_ID_PTR nborGID, int *nborProc,
                                                   int wgt_dim, float *ewgts, int *ierr)
{
  CMeshPartitioner& p = *(CMeshPartitioner *)data;
  *ierr = ZOLTAN_OK;
  
  p.list_of_connected_objects(nborGID);
  p.list_of_connected_procs(nborProc);
	
	
	
	// for debugging
#if 0
  const Uint nb_obj = p.nb_owned_objects();
  std::vector<Uint> numEdges(nb_obj);
  Uint tot_nb_edges = p.nb_connected_objects(numEdges);

  std::vector<Uint> conn_glbID(tot_nb_edges);
  p.list_of_connected_objects(conn_glbID);
  
  std::vector<Uint> glbID(nb_obj);
  p.list_of_owned_objects(glbID);
  
  PEProcessSortedExecute(PE::instance(),-1,
  CFdebug << RANK << "conn_glbID =\n";
  Uint cnt = 0;
  index_foreach( e, const Uint nbEdge, numEdges)
  {
		if (p.is_node(glbID[e]))
			CFdebug << "  N" << p.to_node_glb(glbID[e]) << ": ";
		else
			CFdebug << "  E" << p.to_elem_glb(glbID[e]) << ": ";
		
    for (Uint c=cnt; c<cnt+nbEdge; ++c)
		{
			if (p.is_node(conn_glbID[c]))
				CFdebug << " N" << p.to_node_glb(conn_glbID[c]);
			else
				CFdebug << " E" << p.to_elem_glb(conn_glbID[c]);
		}
    CFdebug << "\n";
    cnt += nbEdge;
  }
  CFdebug << CFendl;
  cf_assert( cnt == tot_nb_edges );
  CFdebug << RANK << "total nb_edges = " << cnt << CFendl;
  )
#endif
	
}
	
//////////////////////////////////////////////////////////////////////

void CPartitioner::migrate()
{
	
	if (property("Number of Partitions").value<Uint>() == 1)
		return;
	
  // replace local node numbers in connectivity table by global node numbers
	
  PE::instance().barrier();
  std::cout << std::flush;
  boost::this_thread::sleep(boost::posix_time::milliseconds(1));
  PE::instance().barrier();  
  
  CFdebug << CFendl;
  CFdebug << "Prepare\n";
  CFdebug << "-------" << CFendl;
	
	boost_foreach (Component::Ptr comp, components_vector())
	{
		// give the element to node connectivity global indices
		if(CElements::Ptr elements = boost::dynamic_pointer_cast<CElements>(comp))
		{
			CTable<Uint>& conn_table = elements->connectivity_table();
			const CTable<Real>& coordinates = elements->coordinates();
			const CList<Uint>& global_node_indices = find_component_with_tag< CList<Uint> >(coordinates,"global_node_indices");
			
			boost_foreach ( CTable<Uint>::Row nodes, conn_table.array() )
			{
				boost_foreach ( Uint& node, nodes )
				{
					node = global_node_indices[node];
				}
			}
		}
	}
	
	PE::instance().barrier();
  std::cout << std::flush;
  boost::this_thread::sleep(boost::posix_time::milliseconds(1));
  PE::instance().barrier();  
  
  CFdebug << CFendl;
  CFdebug << "Migrating elements\n";
  CFdebug << "------------------" << CFendl;
	

	m_zz->Set_Obj_Size_Multi_Fn ( get_elems_sizes , this );
  m_zz->Set_Pack_Obj_Multi_Fn ( pack_elems_messages , this );
  m_zz->Set_Unpack_Obj_Multi_Fn ( unpack_elems_messages , this );
	
  CFdebug.setFilterRankZero(false);

	m_zz->Migrate( numImport, importGlobalIds, importLocalIds, importProcs, importToPart,
								 numExport, exportGlobalIds, exportLocalIds, exportProcs, exportToPart);
	
  CFdebug.setFilterRankZero(true);

  PE::instance().barrier();
  std::cout << std::flush;
  boost::this_thread::sleep(boost::posix_time::milliseconds(1));
  PE::instance().barrier();  
  

  CFdebug << CFendl;
  CFdebug << "Searching for missing nodes\n";
  CFdebug << "---------------------------" <<CFendl;
  
	std::set<Uint> required_nodes;

  std::set<Uint>::iterator it;
  std::set<Uint>::iterator not_found = required_nodes.end();
  
  // 1) put in ghost_nodes initially ALL the nodes required by the migrated elements
  boost_foreach (Component::Ptr comp, components_vector())
  {
    // give the element to node connectivity global indices
    if(CElements::Ptr elements = boost::dynamic_pointer_cast<CElements>(comp))
    {
      CTable<Uint>& conn_table = elements->connectivity_table();
      boost_foreach ( CTable<Uint>::Row nodes, conn_table.array() )
      {
        boost_foreach ( Uint& node, nodes )
        {
          required_nodes.insert(node);
        }
      }
    }
  }
    
  // 2) remove from required_nodes ALL the nodes that are present in the coordinate tables
  boost_foreach (Component::Ptr comp, components_vector())
  {
    // give the element to node connectivity global indices
    if(CTable<Real>::Ptr coordinates = boost::dynamic_pointer_cast<CTable<Real> >(comp))
    {
      const CList<Uint>& global_node_indices = find_component_with_tag< CList<Uint> >(*coordinates,"global_node_indices");
      CList<bool>& is_ghost = find_component_with_tag< CList<bool> >(*coordinates,"is_ghost");      
      for (Uint i=0; i<coordinates->size(); ++i)
      {
        it = required_nodes.find(global_node_indices[i]);
        // delete node from required_nodes if it is found, and mark as owned
        if (it == not_found)
          is_ghost[i] = 1;
        else // if found
        {
          required_nodes.erase(it);
          is_ghost[i] = 0;
        }
      }
    }
  }
  
  int num_request = required_nodes.size();
  ZOLTAN_ID_PTR request_global_ids = new Uint [num_request];
  ZOLTAN_ID_PTR request_local_ids = NULL;
  int* request_from_procs = new int [num_request];
  int* request_to_part = new int [num_request];;
	
	int num_send;
	ZOLTAN_ID_PTR send_global_ids;// = importGlobalIds;
  ZOLTAN_ID_PTR send_local_ids = NULL;//  = importLocalIds;
	int* send_to_procs;// = importProcs;
  int* send_to_part;// = importToPart;
	
	
  Uint idx=0;
  BOOST_FOREACH(const Uint required_node, required_nodes)
  {
    request_global_ids[idx] = from_node_glb(required_node);
    request_to_part[idx] = PE::instance().rank();
    request_from_procs[idx] = proc_of_obj(from_node_glb(required_node));
    ++idx;
  }
	
	
	m_zz->Invert_Lists ( num_request, request_global_ids, request_local_ids, request_from_procs, request_to_part, 
											 num_send,    send_global_ids,    send_local_ids,    send_to_procs,      send_to_part);
	
	// Trick zoltan to accept local id as a boolean to see if it is a ghost or not
	send_local_ids = new Uint[num_send];
	CMap<Uint,Uint>::iterator _it;
	for (int i=0; i<num_send; ++i)
	{
		_it = m_changes->find(send_global_ids[i]);
		if ( _it != m_changes->end())
		{	
			if ( _it->second != (Uint)send_to_part[i] )
				send_local_ids[i] = 1; // is ghost;
			else
				send_local_ids[i] = 0; // is not ghost
		}
		else
			send_local_ids[i] = 1; // is ghost;
	}
	
	CFdebug.setFilterRankZero(false);

	PEProcessSortedExecute(PE::instance(),-1,
                         CFdebug << CFendl;
                         CFdebug << "proc#" << PE::instance().rank() << CFendl;
                         CFdebug << "------"<<CFendl;
                         
	CFdebug << "request nodes: " << CFendl << "     ";
	for (int i=0; i<num_request; ++i)
	{
		CFdebug << to_node_glb(request_global_ids[i]) << "[" << request_from_procs[i] << "]  ";
	}
  CFdebug << CFendl;
												 
	CFdebug << "send nodes: " << CFendl << "     ";
	for (int i=0; i<num_send; ++i)
	{
		CFdebug << to_node_glb(send_global_ids[i]) <<  "[" << send_to_part[i] << "]  ";
//		if (send_local_ids[i])
//			CFdebug << " as ghost ";
//		CFdebug << CFendl;

	}
  CFdebug << CFendl << CFendl;)
	
	CFdebug.setFilterRankZero(true);
	
  PE::instance().barrier();
  std::cout << std::flush;
  boost::this_thread::sleep(boost::posix_time::milliseconds(1));
  PE::instance().barrier();  
  
	
  CFdebug << CFendl;
  CFdebug << "Migrating nodes\n";
  CFdebug << "---------------" << CFendl;
	
	m_zz->Set_Obj_Size_Multi_Fn ( get_nodes_sizes , this );
  m_zz->Set_Pack_Obj_Multi_Fn ( pack_nodes_messages , this);
  m_zz->Set_Unpack_Obj_Multi_Fn ( unpack_nodes_messages , this );
  m_zz->Set_Post_Migrate_PP_Fn ( NULL , this );
  
  CFdebug.setFilterRankZero(false);
	
	m_zz->Migrate( num_request, request_global_ids, request_local_ids, request_from_procs, request_to_part, 
                 num_send,    send_global_ids,    send_local_ids,    send_to_procs,      send_to_part);

  CFdebug.setFilterRankZero(true);

	ZoltanObject::LB_Free_Part(&request_global_ids, &request_local_ids, &request_from_procs, &request_to_part);
  ZoltanObject::LB_Free_Part(&send_global_ids, &send_local_ids, &send_to_procs, &send_to_part);  

		
  PE::instance().barrier();
  std::cout << std::flush;
  boost::this_thread::sleep(boost::posix_time::milliseconds(1));
  PE::instance().barrier();  
  
  CFdebug << CFendl;
  CFdebug << "Cleanup\n";
  CFdebug << "-------" << CFendl;

  // replace back the node numbers in connectivity table by the new local node numbers
  std::map<Uint,Uint> glb_to_loc;
  
  boost_foreach (Component::Ptr comp, components_vector())
  {
    // give the element to node connectivity global indices
    if(CTable<Real>::Ptr coordinates = boost::dynamic_pointer_cast<CTable<Real> >(comp))
    {
      const CList<Uint>& global_node_indices = find_component_with_tag< CList<Uint> >(*coordinates,"global_node_indices");
      cf_assert(global_node_indices.size() == coordinates->size());
      for (Uint i=0; i<coordinates->size(); ++i)
      {
        glb_to_loc[global_node_indices[i]]=i;
      }
    }
  }
  
  boost_foreach (Component::Ptr comp, components_vector())
  {
    // give the element to node connectivity global indices
    if(CElements::Ptr elements = boost::dynamic_pointer_cast<CElements>(comp))
    {
      CTable<Uint>& conn_table = elements->connectivity_table();
      
      BOOST_FOREACH ( CTable<Uint>::Row nodes, conn_table.array() )
      {
        BOOST_FOREACH ( Uint& node, nodes )
        {
          node = glb_to_loc[node];
        }
      }      
    }    
  }
  

}

//////////////////////////////////////////////////////////////////////

void CPartitioner::get_elems_sizes(void *data, int gidSize, int lidSize, int num_ids,
										 ZOLTAN_ID_PTR globalIDs, ZOLTAN_ID_PTR localIDs, int *sizes, int *ierr)
{    
	CMeshPartitioner& p = *(CMeshPartitioner *)data;
	*ierr = ZOLTAN_OK;
	
	Component::Ptr comp;
	Uint idx;

	Uint nb_elems = 0;
	int i=0;
	foreach_container( (const Uint glb_idx) , p.changes() )
	{
		cf_assert(i<num_ids);
		if ( p.is_node(glb_idx) )
		{
			sizes[i] = 0;
		}
		else
		{
			nb_elems++;
			boost::tie(comp,idx) = p.to_local(glb_idx);
			sizes[i] = sizeof(Uint) // component index
							 + sizeof(Uint) * comp->as_type<CElements>()->connectivity_table().row_size(); // nodes
		}
		++i;
	}
	cf_assert(i==num_ids);
}

//////////////////////////////////////////////////////////////////////

void CPartitioner::pack_elems_messages(void *data, int gidSize, int lidSize, int num_ids,
												 ZOLTAN_ID_PTR globalIDs, ZOLTAN_ID_PTR localIDs, int *dests, int *sizes, int *idx, char *buf, int *ierr)
{
	
	CMeshPartitioner& p = *(CMeshPartitioner *)data;
	*ierr = ZOLTAN_OK;
	
	Uint comp_idx;
	Uint array_idx;
	bool is_found;
	
	std::vector<boost::shared_ptr<CTable<Uint>::Buffer> > elem_buffer;
	boost_foreach (Component::Ptr comp, p.components_vector())
	{
		if(CElements::Ptr elements = boost::dynamic_pointer_cast<CElements>(comp))
			elem_buffer.push_back(boost::shared_ptr<CTable<Uint>::Buffer> ( new CTable<Uint>::Buffer(elements->connectivity_table().create_buffer())));
		else
			elem_buffer.push_back(boost::shared_ptr<CTable<Uint>::Buffer>());
	}
	
	Uint* comp_idx_buf;
	Uint* nodes_buf;
	
	Uint i=0;
	foreach_container( (const Uint glb_idx) , p.changes() )
	{
		if ( p.is_node(glb_idx) )
		{
			// don't pack nodes
		}
		else
		{
			boost::tie(comp_idx,array_idx,is_found) = p.to_local_indices_from_glb_obj(glb_idx);
			
			comp_idx_buf = (Uint *)(buf + idx[i]);
			*comp_idx_buf++ = comp_idx;
			
			CFdebug << RANK << "packed elem " << p.to_elem_glb(glb_idx) << " : ";
			nodes_buf = (Uint *)(comp_idx_buf);
			boost_foreach (const Uint node, p.components_vector()[comp_idx]->as_type<CElements>()->connectivity_table()[array_idx])
			{
				CFdebug << " " << node;
				*nodes_buf++ = node;
			}
			CFdebug << CFendl;
			
			elem_buffer[comp_idx]->rm_row(array_idx);
			CFdebug << RANK << "removed elem " << p.to_elem_glb(glb_idx) << CFendl;
			
		}
		++i;
	}
	
}

//////////////////////////////////////////////////////////////////////

void CPartitioner::unpack_elems_messages(void *data, int gidSize, int num_ids,
																	ZOLTAN_ID_PTR globalIDs, int *sizes, int *idx, char *buf, int *ierr)
{
	
	CPartitioner& p = *(CPartitioner *)data;
	*ierr = ZOLTAN_OK;
	
	
	Uint comp_idx;
	
	std::vector<boost::shared_ptr<CTable<Uint>::Buffer> > elem_buffer;
	boost_foreach (Component::Ptr comp, p.components_vector())
	{
		if(CElements::Ptr elements = boost::dynamic_pointer_cast<CElements>(comp))
		{
			elem_buffer.push_back(boost::shared_ptr<CTable<Uint>::Buffer> ( new CTable<Uint>::Buffer(elements->connectivity_table().create_buffer())));

		}
		else
			elem_buffer.push_back(boost::shared_ptr<CTable<Uint>::Buffer>());
	}
	
	Uint* comp_idx_buf;
	Uint* nodes_buf;

	
	for (int id=0; id < num_ids; id++) 
	{
		int glb_obj = *(int*)(globalIDs+id*gidSize);
		
		if ( !p.is_node(glb_obj))
		{	
			comp_idx_buf = (Uint *)(buf + idx[id]);
			comp_idx = *comp_idx_buf;
			
			Uint nb_nodes = p.components_vector()[comp_idx]->as_type<CElements>()->connectivity_table().row_size();
			std::vector<Uint> nodes(nb_nodes);
			
			nodes_buf = (Uint *)(++comp_idx_buf);
			for (Uint i=0; i<nb_nodes; ++i)
			{
				nodes[i] = *nodes_buf++;
			}
			
			elem_buffer[comp_idx]->add_row(nodes);

			CFdebug << RANK << "unpacked elem " << p.to_elem_glb(glb_obj) << CFendl;
		}
		
	}
					
}

//////////////////////////////////////////////////////////////////////

void CPartitioner::get_nodes_sizes(void *data, int gidSize, int lidSize, int num_ids,
																	 ZOLTAN_ID_PTR globalIDs, ZOLTAN_ID_PTR localIDs, int *sizes, int *ierr)
{
	
	CMeshPartitioner& p = *(CMeshPartitioner *)data;
	*ierr = ZOLTAN_OK;
	
	Component::Ptr comp;
	Uint comp_idx;
	Uint array_idx;
	bool is_found;
	for (int i=0; i < num_ids; i++) 
	{
		Uint glb_idx = *(int*)(globalIDs+i*gidSize);
		
		boost::tie(comp_idx,array_idx,is_found) = p.to_local_indices_from_glb_obj(glb_idx);
		comp = p.components_vector()[comp_idx];
		sizes[i] = sizeof(Uint) // component index
		+ sizeof(bool) // send_as_ghost true/false
		+ sizeof(Uint) // gid
		+ sizeof(Real) * comp->as_type<CTable<Real> >()->row_size() // coordinates
		+ sizeof(Uint) * (1+p.connectivity_components_vector()[comp_idx]->as_type<CDynTable<Uint> >()->row_size(array_idx)); // global element indices that need this node
	}		
}

//////////////////////////////////////////////////////////////////////

void CPartitioner::pack_nodes_messages(void *data, int gidSize, int lidSize, int num_ids,
																			 ZOLTAN_ID_PTR globalIDs, ZOLTAN_ID_PTR localIDs, int *dests, int *sizes, int *idx, char *buf, int *ierr)
{
	CMeshPartitioner& p = *(CMeshPartitioner *)data;
	*ierr = ZOLTAN_OK;

	std::vector<CTable<Real>::Ptr> nodes;
	std::vector<CDynTable<Uint>::Ptr> connectivity;
	std::vector<CList<bool>::Ptr> is_ghost;
	std::vector<CList<Uint>::Ptr> gids;
	boost_foreach (Component::Ptr comp, p.components_vector())
	{
		if(CTable<Real>::Ptr coordinates = boost::dynamic_pointer_cast<CTable<Real> >(comp))
		{
			nodes.push_back(coordinates);
			connectivity.push_back(find_component_ptr_with_tag<CDynTable<Uint> >(*coordinates,"glb_elem_connectivity"));
			gids.push_back(find_component_ptr_with_tag< CList<Uint> >(*coordinates,"global_node_indices"));
			is_ghost.push_back(find_component_ptr_with_tag< CList<bool> >(*coordinates,"is_ghost"));
		}
		else
		{
			nodes.push_back(CTable<Real>::Ptr());
			connectivity.push_back(CDynTable<Uint>::Ptr());
			gids.push_back(CList<Uint>::Ptr());
			is_ghost.push_back(CList<bool>::Ptr());
		}
	}
	
	Component::Ptr comp;
	Uint comp_idx;
	Uint array_idx;
	bool is_found;
	
	Uint* comp_idx_buf;
	bool* as_ghost_buf;
	Uint* gid_buf;
	Real* coords_buf;
	Uint* elems_buf;

	for (int i=0; i < num_ids; i++) 
	{
		Uint glb_idx = *(int*)(globalIDs+i*gidSize);
		bool send_as_ghost = *(int*)(localIDs+i);
		
		boost::tie(comp_idx,array_idx,is_found) = p.to_local_indices_from_glb_obj(glb_idx);
		comp = p.components_vector()[comp_idx];
		
		comp_idx_buf = (Uint *)(buf + idx[i]);
		*comp_idx_buf++ = comp_idx;
		
		as_ghost_buf = (bool *)(comp_idx_buf);
		*as_ghost_buf++ = send_as_ghost;
		
		gid_buf = (Uint *)(as_ghost_buf);
		*gid_buf++ = gids[comp_idx]->array()[array_idx];
		
		coords_buf = (Real *)(gid_buf);
		boost_foreach (const Real& node, nodes[comp_idx]->array()[array_idx])
		{
			*coords_buf++ = node;
		}
		
		elems_buf = (Uint *)(coords_buf);
		*elems_buf++ = connectivity[comp_idx]->row_size(array_idx);
		boost_foreach (const Uint elem, connectivity[comp_idx]->array()[array_idx])
		{
			*elems_buf++ = elem;
		}
		
		CFdebug << RANK << "packed node " << p.to_node_glb(glb_idx) << CFendl;
	}
			
	for(Uint b=0; b<gids.size(); ++b)
	{
		if (is_not_null(gids[b]))
		{
			CTable<Real>::Buffer node_buffer = nodes[b]->create_buffer();
			CList<Uint>::Buffer gid_buffer = gids[b]->create_buffer();
			CList<bool>::Buffer is_ghost_buffer = is_ghost[b]->create_buffer();
			CDynTable<Uint>::Buffer connectivity_buffer = connectivity[b]->create_buffer();
			for (Uint i=0; i<gids[b]->size(); ++i)
			{
				// if this gid is not found in required nodes
				if (is_ghost[b]->array()[i])
				{
					Uint gid = gids[b]->array()[i];
					gid_buffer.rm_row(i);
					node_buffer.rm_row(i);
					is_ghost_buffer.rm_row(i);
					connectivity_buffer.rm_row(i);
					CFdebug << RANK << "removed node " << gid << "("<<i<<")"<< CFendl;
				}
			}

			node_buffer.flush();
			gid_buffer.flush();
			is_ghost_buffer.flush();
			connectivity_buffer.flush();
		}
	}
}

//////////////////////////////////////////////////////////////////////

void CPartitioner::unpack_nodes_messages(void *data, int gidSize, int num_ids,
																				 ZOLTAN_ID_PTR globalIDs, int *sizes, int *idx, char *buf, int *ierr)
{
	CMeshPartitioner& p = *(CMeshPartitioner *)data;
	*ierr = ZOLTAN_OK;

	
	std::vector<boost::shared_ptr<CTable<Real>::Buffer> > node_buffer;
	std::vector<boost::shared_ptr<CDynTable<Uint>::Buffer> > connectivity_buffer;
	std::vector<boost::shared_ptr<CList<bool>::Buffer> > is_ghost_buffer;
	std::vector<boost::shared_ptr<CList<Uint>::Buffer> > gid_buffer;
	boost_foreach (Component::Ptr comp, p.components_vector())
	{
		if(CTable<Real>::Ptr coordinates = boost::dynamic_pointer_cast<CTable<Real> >(comp))
		{
			CDynTable<Uint>& conn_table = find_component_with_tag<CDynTable<Uint> >(*coordinates,"glb_elem_connectivity");
			CList<Uint>& gid = find_component_with_tag< CList<Uint> >(*coordinates,"global_node_indices");
			CList<bool>& is_ghost = find_component_with_tag< CList<bool> >(*coordinates,"is_ghost");
			
			node_buffer.push_back(boost::shared_ptr<CTable<Real>::Buffer> ( new CTable<Real>::Buffer(coordinates->create_buffer())) );
			connectivity_buffer.push_back(boost::shared_ptr<CDynTable<Uint>::Buffer> ( new CDynTable<Uint>::Buffer(conn_table.create_buffer())) );
			is_ghost_buffer.push_back(boost::shared_ptr<CList<bool>::Buffer> ( new CList<bool>::Buffer(is_ghost.create_buffer())) );
			gid_buffer.push_back(boost::shared_ptr<CList<Uint>::Buffer> ( new CList<Uint>::Buffer(gid.create_buffer())) );
		}
		else
		{
			node_buffer.push_back(boost::shared_ptr<CTable<Real>::Buffer>() );
			connectivity_buffer.push_back(boost::shared_ptr<CDynTable<Uint>::Buffer>() );
			is_ghost_buffer.push_back(boost::shared_ptr<CList<bool>::Buffer>() );
			gid_buffer.push_back(boost::shared_ptr<CList<Uint>::Buffer>() );
		}
	}

	
	Component::Ptr comp;
	Uint comp_idx;
	Uint receive_as_ghost;
	Uint gid;
	Uint nb_elems;
	std::vector<Uint> elems;
	
	Uint* comp_idx_buf;
	bool* as_ghost_buf;
	Uint* gid_buf;
	Real* coords_buf;
	Uint* elems_buf;
	
	Uint loc, check_loc;
	
	for (int i=0; i < num_ids; i++) 
	{
		comp_idx_buf = (Uint *)(buf + idx[i]);
		comp_idx = *comp_idx_buf++ ;
		comp = p.components_vector()[comp_idx];
		
		as_ghost_buf = (bool *)(comp_idx_buf);
		receive_as_ghost = *as_ghost_buf++ ;
		
		gid_buf = (Uint *)(as_ghost_buf);
		gid = *gid_buf++;
		
		coords_buf = (Real *)(gid_buf);
		std::vector<Real> coords(comp->as_type<CTable<Real> >()->row_size());
		boost_foreach (Real& coord, coords)
			coord = *coords_buf++;
		
		elems_buf = (Uint *)(coords_buf);
		nb_elems = *elems_buf++;
		elems.resize(nb_elems);
		boost_foreach (Uint& elem, elems)
			elem = *elems_buf++;
		
		loc = node_buffer[comp_idx]->add_row(coords);
		CFdebug << RANK << "unpacked node " << gid << " (";
		for (Uint j=0; j<coords.size()-1; ++j)
			CFdebug << coords[j] << ",";
		CFdebug << coords.back() << ")";
		if (receive_as_ghost)
			CFdebug << " as ghost";
		CFdebug << " to location " << loc;
		CFdebug << CFendl;

		check_loc = gid_buffer[comp_idx]->add_row(gid);
		cf_assert( check_loc == loc );

		check_loc = connectivity_buffer[comp_idx]->add_row(elems);
		cf_assert( check_loc == loc );
		
		check_loc = is_ghost_buffer[comp_idx]->add_row(receive_as_ghost);
		cf_assert( check_loc == loc );

	}
	
}
	
//////////////////////////////////////////////////////////////////////////////

} // Zoltan
} // Mesh
} // CF
