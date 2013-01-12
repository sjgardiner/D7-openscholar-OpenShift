<?php

/**
 * Implements hook_views_pre_view().
 */
function vsite_access_views_pre_view(&$view, &$display_id, &$args){

	$space = spaces_get_space();
	
	//If we are not in a vsite, add our filter, inside a vsite we can restrict access without filters overhead
	if (arg(0) === 'admin' || !is_object($space)) {
		
		//After we have determined that we are not in an OG space lets make
		//sure we add our filters
		$group_filter = $view->get_item($display_id, 'filter', VSITE_ACCESS_FIELD.'_value');

		//If the group filter is already present there is no need to do anything
		if (!$group_filter) {

			$table = $view->base_table;

			if (in_array($table, array(
					'node'
			))) {
				if (! $view->get_item($display_id, 'relationship', 'og_membership_rel')) {
					//lets add the Relationship to the og table
					$view->add_item($display_id, 'relationship', $table, 'og_membership_rel');
				}
				
				//Lets add the relationship to the group node
				if (! $view->get_item($display_id, 'relationship', 'og_membership_related_node_group')) {
					//lets add the Relationship to the og table
					$view->add_item($display_id, 'relationship', 'og_membership', 'og_membership_related_node_group');
				}
				
				$group_filter = array(
						'id' => 'group_access_value',
						'table' => 'field_data_group_access',
						'field' => 'group_access_value',
						'group_type' => 'group',
						'operator' => 'not in',
						'group' => '1',
						'exposed' => false,
						'relationship' => 'og_membership_related_node_group',
						'value' => array('1' => '1'),
				);
				
				//Lets add the filter for group access field
				$view->set_item($display_id, 'filter', VSITE_ACCESS_FIELD.'_value', $group_filter);

			}


		}
	}else if(!og_is_member('node', $space->id)){
		//The None of the view content is visible if the space is private
	}
}