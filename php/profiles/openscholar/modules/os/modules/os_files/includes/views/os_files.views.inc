<?php

/**
 * @file
 * Views implementation for os files.
 */

/**
 * Implements hook_views_data().
 */
function os_files_views_data() {
  $data = array();

  $data['file_managed']['os_files_copy_link'] = array(
    'field' => array(
      'field' => 'uri',
      'title' => t('Copy link'),
      'help' => t('Copy link path.'),
      'handler' => 'os_files_handler_field_file_uri_copy',
    ),
  );

  return $data;
}
