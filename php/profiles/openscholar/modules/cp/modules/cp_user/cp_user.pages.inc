<?php
/**
 * Function to return the user's role in the group,
 * along with the links to perform action based on the returned roles.
 */
function cp_user_check_role($user_id, $group_id) {
  // First check to see if the user is the admin of this group.
  $space = spaces_get_space();

  if($space->group->nid == $group_id && $space->group->uid == $user_id){
	// User is the manager of the site, don't allow any actions on them.
	$group_member = t('Manager');
	$admin_link = '';
	$remove_link = '';
  }else{
  	// Check their OG roles here, they could be a vsite admin and not the manager
	$user_roles = og_get_user_roles('node',$space->group->nid, $user_id);
	// Check for the vsite admin role now
	if(in_array('vsite admin', $user_roles)) {
	  $group_member = t('Administrator');
      $admin_link = l('Remove Admin',"cp/users/remove_admin/{$user_id}");
	}else {
	  $group_member = t('Member');
	  $admin_link = l('Make Admin',"cp/users/create_admin/{$user_id}");
	}
	$remove_link = l('Remove Membership',"cp/users/delete_membership/{$user_id}");
  }
  $group_membership['membership'] = $group_member;
  $group_membership['admin_link'] = $admin_link;
  $group_membership['remove_link'] = $remove_link;

  return $group_membership;
}

/**
 * Function to return the user's first/last names.
 */
function cp_user_get_user_names($user_id) {
  $user = user_load($user_id);
  $name_array = array();
  // Get the first name.
  $user_first_name = field_get_items('user', $user, 'field_first_name');
  if(isset($user_first_name[0]['safe_value'])) {
    $first_name = $user_first_name[0]['safe_value'];
  }else{
  	$first_name = '';
  }
  // Get the last name.
  $user_last_name = field_get_items('user', $user, 'field_last_name');
  if(isset($user_last_name[0]['safe_value'])) {
    $last_name = $user_last_name[0]['safe_value'];
  }else {
  	$last_name = '';
  }
  $name_array['username'] = $user->name;
  $name_array['first_name'] = $first_name;
  $name_array['last_name'] = $last_name;

  return $name_array;
}

/**
 * Function to return the users of a group.
 */
function cp_user_load_group_users($vsite_id) {
  $query = db_select("og_membership", "ogm");
  $query->condition("ogm.gid", $vsite_id, "=");
  $query->condition("entity_type","user","=");
  $query->fields("ogm", array("entity_type", "etid"));
  $result = $query->execute()->fetchAll();

  $user_array = array();
  foreach($result as $entity=>$value) {
    $user_array[] = array('User'=>cp_user_get_user_names($value->etid),'Member'=>cp_user_check_role($value->etid, $vsite_id));
  }
  return $user_array;
}

/**
 * Function to display the users of group,
 * action links for those users as well as the
 * add new user link.
 */
function cp_user_user_management_page($vsite_id) {
  // Create the table.
  $rows = array();
  $header = array('Username','First Name','Last Name','Role','Admin','Manage');
  $data_load = cp_user_load_group_users($vsite_id);
  foreach($data_load as $data=>$load) {
    $rows[] = array($load['User']['username'],
      $load['User']['first_name'],
	  $load['User']['last_name'],
	  $load['Member']['membership'],
	  $load['Member']['admin_link'],
	  $load['Member']['remove_link'],
    );
  }
  $add_new_link = l('+ Add a new user','cp/users/add');

  $output = '<div class="cp-manage-users-wrapper"><div class="cp-add-new-user"><ul><li>'.$add_new_link.'</li></ul></div><div class="cp-manager-user-content">';
  $output .= theme('table',array('header'=>$header,'rows'=>$rows));
  $output .= '</div></div>';

  return $output;
}

/**
 * Implements hook_confirm_form().
 * Confirmation form for removing the membership of the user form the specified group.
 */
function cp_user_remove_membership_confirm($form_state, $user_load) {
  $space = spaces_get_space();
  // Form sets.
  $form['gid'] = array('#type' => 'value', '#value' => $space->group->nid);
  $form['account'] = array('#type' => 'value', '#value' => $user_load['build_info']['args'][0]->uid);
  $form['username'] = array('#type' => 'value', '#value' => $user_load['build_info']['args'][0]->name);
  $form['site_name'] = array('#type' => 'value', '#value' => $space->group->title);
  // Return hook_confirm_form();
  return confirm_form($form,
    t('Are you sure you want to remove !name from the website: %title?', array('!name' => $user_load['build_info']['args'][0]->name, '%title' => $space->group->title)),
    "cp/users",
    ' ',
    t('Remove'),
    t('Cancel')
  );
}

/**
 * Function to remove the user from the selected group/site.
 */
function cp_user_remove_membership_confirm_submit($form, &$form_state) {
  // Ungroup the user from the node
  og_ungroup('node',$form_state['values']['gid'],'user',$form_state['values']['account']);
  // Set the message and redirect back to the cp/users page
  drupal_set_message(t('!user has been removed from !website', array('!user'=>$form_state['values']['username'],'!website'=>$form_state['values']['site_name'])));
  drupal_goto('cp/users');
}

/**
 * Implements hook_confirm_form().
 * Confirmation form for promoting a user to vsite admin of the specified group.
 */
function cp_user_make_admin_confirm($form_state, $user_load) {
  $space = spaces_get_space();
  // Form sets.
  $form['gid'] = array('#type' => 'value', '#value' => $space->group->nid);
  $form['group_type'] = array('#type' => 'value', '#value' => $space->group->type);
  $form['account'] = array('#type' => 'value', '#value' => $user_load['build_info']['args'][0]->uid);
  $form['username'] = array('#type' => 'value', '#value' => $user_load['build_info']['args'][0]->name);
  $form['site_name'] = array('#type' => 'value', '#value' => $space->group->title);
  // Return hook_confirm_form();
  return confirm_form($form,
    t('Are you sure you want to make !name a group administrator for the website: %title?', array('!name' => $user_load['build_info']['args'][0]->name, '%title' => $space->group->title)),
    "cp/users",
    ' ',
    t('Confirm'),
    t('Cancel')
  );
}

/**
 * Function to promote the user to vsite admin of the selected group/site.
 */
function cp_user_make_admin_confirm_submit($form, &$form_state) {
  // Get the vsite admin key.
  $og_roles = og_roles('node', $form_state['values']['group_type']);
  $key = array_search('vsite admin', $og_roles);
  $vsite_admin_role = $key;

  // Grant them the admin role
  og_role_grant('node', $form_state['values']['gid'], $form_state['values']['account'], $vsite_admin_role);
  // Set the message and redirect back to the cp/users page
  drupal_set_message(t('!user has been promoted to administrator for !website', array('!user'=>$form_state['values']['username'],'!website'=>$form_state['values']['site_name'])));
  drupal_goto('cp/users');
}

/**
 * Implements hook_confirm_form().
 * Confirmation form for promoting a user to vsite admin of the specified group.
 */
function cp_user_remove_admin_confirm($form_state, $user_load) {
  $space = spaces_get_space();
  // Form sets.
  $form['gid'] = array('#type' => 'value', '#value' => $space->group->nid);
  $form['group_type'] = array('#type' => 'value', '#value' => $space->group->type);
  $form['account'] = array('#type' => 'value', '#value' => $user_load['build_info']['args'][0]->uid);
  $form['username'] = array('#type' => 'value', '#value' => $user_load['build_info']['args'][0]->name);
  $form['site_name'] = array('#type' => 'value', '#value' => $space->group->title);
  // Return hook_confirm_form();
  return confirm_form($form,
    t('Are you sure you want to remove !name as a group administrator for the website: %title?', array('!name' => $user_load['build_info']['args'][0]->name, '%title' => $space->group->title)),
    "cp/users",
    ' ',
    t('Remove'),
    t('Cancel')
  );
}

/**
 * Function to promote the user to vsite admin of the selected group/site.
 */
function cp_user_remove_admin_confirm_submit($form, &$form_state) {
  // Get the vsite admin key.
  $og_roles = og_roles('node', $form_state['values']['group_type']);
  $key = array_search('vsite admin', $og_roles);
  $vsite_admin_role = $key;

  // Revoke the admin role
  og_role_revoke('node', $form_state['values']['gid'], $form_state['values']['account'], $vsite_admin_role);
  // Set the message and redirect back to the cp/users page
  drupal_set_message(t('!user has been removed as administrator for !website', array('!user'=>$form_state['values']['username'],'!website'=>$form_state['values']['site_name'])));
  drupal_goto('cp/users');
}
