<?php
/**
 * @file
 * os.spaces.inc
 */

/**
 * Implements hook_spaces_presets().
 */
function os_spaces_presets() {
  $export = array();

  $spaces_presets = new stdClass();
  $spaces_presets->disabled = FALSE; /* Edit this to true to make a default spaces_presets disabled initially */
  $spaces_presets->api_version = 3;
  $spaces_presets->name = 'os_department';
  $spaces_presets->title = 'Academic Department';
  $spaces_presets->description = '';
  $spaces_presets->space_type = 'og';
  $spaces_presets->value = array(
    'bundles' => array('department'),
    'variable' => array(
      'spaces_features' => array(
        'os' => '1',
        'os_pages' => '1',
        'os_news' => '1',
        'os_events' => '1',
        'os_profiles' => '1',
        'os_links' => '1',
      ),
      'spaces_features_available' => array(
        'os_blog',
        'os_booklets',
        'os_classes',
        'os_dataverse',
        'os_events',
        'os_faq',
        'os_files_private',
        'os_image_gallery',
        'os_links',
        'os_news',
        'os_pages',
        'os_presentations',
        'os_profiles',
        'os_publications',
        'os_reader',
        'os_software',
        'harvard_courses'
      ),
      'site_frontpage' => 'home',
      'theme_default' => 'ballard',
    ),
  );

  // Translatables
  // Included for use with string extractors like potx.
  t('OS Department');
  $export['os_department'] = $spaces_presets;

  $spaces_presets = new stdClass();
  $spaces_presets->disabled = FALSE; /* Edit this to true to make a default spaces_presets disabled initially */
  $spaces_presets->api_version = 3;
  $spaces_presets->name = 'os_department_minimal';
  $spaces_presets->title = 'Department Minimal';
  $spaces_presets->description = '';
  $spaces_presets->space_type = 'og';
  $spaces_presets->value = array(
    'bundles' => array('department'),
    'variable' => array(
      'spaces_features' => array(
        'os' => '1',
        'os_pages' => '1',
      ),
      'spaces_features_available' => array(
        'os_blog',
        'os_booklets',
        'os_classes',
        'os_dataverse',
        'os_events',
        'os_faq',
        'os_files_private',
        'os_image_gallery',
        'os_links',
        'os_news',
        'os_pages',
        'os_presentations',
        'os_profiles',
        'os_publications',
        'os_reader',
        'os_software',
        'harvard_courses',
      ),
      'site_frontpage' => 'home',
      'theme_default' => 'ballard',
    ),
  );

  // Translatables
  // Included for use with string extractors like potx.
  t('OS Department Minimal');
  $export['os_department_minimal'] = $spaces_presets;

  $spaces_presets = new stdClass();
  $spaces_presets->disabled = FALSE; /* Edit this to true to make a default spaces_presets disabled initially */
  $spaces_presets->api_version = 3;
  $spaces_presets->name = 'os_scholar';
  $spaces_presets->title = 'OS Scholar';
  $spaces_presets->description = '';
  $spaces_presets->space_type = 'og';
  $spaces_presets->value = array(
    'bundles' => array('personal'),
    'variable' => array(
      'spaces_features' => array(
        'os' => '1',
        'os_pages' => '1',
        'os_classes' => '1',
        'os_publications' => '1',
        'os_biocv' => 1,
      ),
      'spaces_features_available' => array(
        'os_biocv',
        'os_blog',
        'os_booklets',
        'os_classes',
        'os_dataverse',
        'os_events',
        'os_faq',
        'os_files_private',
        'os_image_gallery',
        'os_links',
        'os_news',
        'os_pages',
        'os_presentations',
        'os_profiles',
        'os_publications',
        'os_reader',
        'os_software',
      ),
      'site_frontpage' => 'home',
      'theme_default' => 'cleanblue',
    ),
  );

  // Translatables
  // Included for use with string extractors like potx.
  t('OS Scholar');
  $export['os_scholar'] = $spaces_presets;

  $spaces_presets = new stdClass();
  $spaces_presets->disabled = FALSE; /* Edit this to true to make a default spaces_presets disabled initially */
  $spaces_presets->api_version = 3;
  $spaces_presets->name = 'os_project';
  $spaces_presets->title = 'OS Project';
  $spaces_presets->description = '';
  $spaces_presets->space_type = 'og';
  $spaces_presets->value = array(
    'bundles' => array('project'),
    'variable' => array(
      'spaces_features' => array(
        'os' => '1',
        'os_pages' => '1',
        'os_news' => '1',
        'os_events' => '1',
        'os_profiles' => '1',
        'os_links' => '1',
      ),
      'spaces_features_available' => array(
        'os_blog',
        'os_booklets',
        'os_classes',
        'os_dataverse',
        'os_events',
        'os_faq',
        'os_files_private',
        'os_image_gallery',
        'os_links',
        'os_news',
        'os_pages',
        'os_presentations',
        'os_profiles',
        'os_publications',
        'os_reader',
        'os_software',
      ),
      'site_frontpage' => 'home',
      'theme_default' => 'cleanblue',
    ),
  );

  // Translatables
  // Included for use with string extractors like potx.
  t('OS Project');
  $export['os_project'] = $spaces_presets;

  return $export;
}
