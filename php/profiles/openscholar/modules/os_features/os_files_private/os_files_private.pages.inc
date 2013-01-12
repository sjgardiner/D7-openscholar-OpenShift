<?php
// $Id$

/**
 * @file
 * OpenScholar control panel looks here for feature settings form.
 */

/**
 * Provides settings form for this feature on the Control Panel.
 */
function os_files_private_settings() {
  $form = array();

  $form['os_files_private_link'] = array(
    '#type' => 'markup',
    '#prefix' => '<p>'
      . t('Private files are only accessible to your ' . l(t('site members'), '/cp/users')) . '.'
      . '</p>'
      . '<p>'
      . l(t('Add and manage private files'), 'file-list')
      . '</p>',
  );
  if (module_exists('pinserver')) {
    $form['os_files_private_pinserver'] = array(
      '#type' => 'checkbox',
      '#title' => t('Extend access to Harvard Community'),
      '#default_value' => variable_get('os_files_private_pinserver', 0),
      '#options' => $options,
      '#description' => t('By default, private files are only accessible to your site members. By extending access to Harvard Community, anyone with a valid Harvard XID can log in using the Harvard Pinserver to access your private files.'),
      '#weight' => 1,
    );
  }

  return system_settings_form($form);
}