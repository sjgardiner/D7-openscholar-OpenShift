<?php 

function vsite_register_admin_form($form, &$state) {
  $presets = _vsite_register_list_presets();
  $form = array(
    'os_enabled_spaces_presets' => array(
      '#title' => t('Enabled presets'),
      '#type' => 'checkboxes',
      '#description' => t('Presets provide default settings and features for different types of sites.'),
      '#default_value' => variable_get('os_enabled_spaces_presets', array_combine(array_keys($presets), array_keys($presets))),
      '#options' => $presets, 
    ),
    //vsites per user
    'os_vsites_per_user' => array(
      '#title' => t('Vsites per user'),
      '#type' => 'textfield',
      '#default_value' => variable_get('os_vsites_per_user', 1),
    )
  );
  
  return system_settings_form($form);
}

function vsite_register_admin_form_validate(&$form, &$state) {
  $state['values'];
  if ((int) $state['values']['os_vsites_per_user'] < 0) {
    form_set_error('os_vsites_per_user', t('Vsites per user must be a positive integer.  To restrict users from creating sites, please edit permissions instead.'));
  }
  
  if (!count(array_filter($state['values']['os_enabled_spaces_presets']))) {
    form_set_error('os_enabled_spaces_presets', t('At least one preset must be enabled.'));
  }
}

function _vsite_register_list_presets() {
  $presets = array();
  foreach(spaces_preset_load() as $name => $preset) {
    //if ($preset->export_module != 'spaces_og') {
    if (!(isset($preset->disabled) && $preset->disabled)) {
      $presets[$name] = $preset->title;
    }
  }
  return $presets;
}

function _vsite_register_list_presets_default() {
  $defaults = array();
  foreach (array_keys(_vsite_register_list_presets()) as $key) {
    $defaults[$key] = TRUE;
  } 
  return $defaults;
}

