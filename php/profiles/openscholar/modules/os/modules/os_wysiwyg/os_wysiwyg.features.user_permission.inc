<?php
/**
 * @file
 * os_wysiwyg.features.user_permission.inc
 */

/**
 * Implements hook_user_default_permissions().
 */
function os_wysiwyg_user_default_permissions() {
  $permissions = array();

  // Exported permission: use text format filtered_html.
  $permissions['use text format filtered_html'] = array(
    'name' => 'use text format filtered_html',
    'roles' => array(
      0 => 'anonymous user',
      1 => 'authenticated user',
      2 => 'administrator',
      3 => 'manager',
      4 => 'vsite admin',
      5 => 'vsite user',
    ),
    'module' => 'filter',
  );

  // Exported permission: use text format full_html.
  $permissions['use text format full_html'] = array(
    'name' => 'use text format full_html',
    'roles' => array(),
    'module' => 'filter',
  );

  return $permissions;
}
