<?php

/**
 * @file os_migrate.admin.inc
 *
 * Provides form for OpenScholar migrate settings
 */

/**
 * os_migrate's admin form
 */
function os_migrate_admin() {
  $form = array();
  $form['os_migrate_database'] = array(
    '#title' => t('Migration database'),
    '#description' => t('Drupal 6 database from which to migrate data.'),
    '#type' => 'textfield',
    '#required' => TRUE,
    '#default_value' => variable_get('os_migrate_database', ''),
  );

  $form['#validate'][] = 'os_migrate_admin_validate';

  $form['settings_help'] = array(
    '#title' => t('settings.php configuration'),
    '#collapsible' => TRUE,
    '#collapsed' => TRUE,
    '#type' => 'fieldset',
    '#description' => <<<EOF
<p>Your settings.php file needs to define a connection to the Drupal 6 database.  Please ensure that
in addition to the default connection to your Drupal 7 database, settings.php includes a connection
to the database you specifed.  It will look something like the text below, but with the bold values
replaced with those applicable to your DB.  </p>

The Migration database setting on this page corresponds to the <strong>database</strong> entry below
as specified in your settings.php file.

<pre>
\$databases['<strong>database</strong>']['default'] = array(
  'database' => '<strong>db name</strong>',
  'username' => '<strong>db user</strong>',
  'password' => '<strong>db password</strong>',
  'host' => 'localhost',
  'port' => '',
  'driver' => 'mysql',
  'prefix' => '',
);
</pre>
EOF
  );

  $form['os_migrate_files'] = array(
    '#title' => t('Files directory'),
    '#description' => t('Path to your Drupal 6 installation\'s files.  Migrate will use this path for copying uploaded files (e.g. /var/www/openscholar/)'),
    '#type' => 'textfield',
    '#required' => TRUE,
    '#default_value' => variable_get('os_migrate_files', ''),
  );


  $oembedembedly_api_key = variable_get('oembedembedly_api_key', FALSE);
  if (!$oembedembedly_api_key) {
    $form['embed_key'] = array(
      '#type' => 'markup',
      '#markup' => '<div class="warning messages">Warning.  You have not set an embedly key.  Please configure embedly with an API key if you have many inline embeds to migrate.</div>',
    );
  }

  $form['dev_fieldset'] = array(
    '#title' => t('Development Settings'),
    '#type' => 'fieldset',
    '#collapsed' => FALSE,
    '#collapsible' => TRUE,
  );

  $form['dev_fieldset']['os_migrate_suppress_email'] = array(
    '#title' => t('Email Suppression'),
    '#description' => t('Creating a user account during migration will send that user an email.  Leaving this checked will suppress emails during user migration.  It will also block OpenScholar\'s Mailchimp subscription.'),
    '#type' => 'checkbox',
    '#default_value' => variable_get('os_migrate_suppress_email', TRUE),
  );

  $og_bundles = og_get_all_group_bundle();
  $form['dev_fieldset']['os_migrate_default_group_bundle'] = array(
    '#title' => t('Group Bundle'),
    '#type' => 'select',
    '#description' => t('Vsite content types have changed.  This option selects which content type your vsites will migrate into.  By default, vsites will become personal and projects will remain projects.'),
    '#options' => array('keep' => t('Keep original type'), 'default' => t('Use site\'s default of @type', array('@type'=>variable_get('os_default_group_bundle', 'personal'))))
      + $og_bundles['node'],
    '#default_value' => variable_get('os_migrate_default_group_bundle', 'keep'),
  );

  $form['dev_fieldset']['os_migrate_ignore_dependencies'] = array(
    '#title' => t('Ignore Dependencies'),
    '#description' => t('Migration dependencies will be ignored by default.'),
    '#type' => 'checkbox',
    '#default_value' => variable_get('os_migrate_ignore_dependencies', FALSE),
  );

  $form['dev_fieldset']['os_migrate_force_og'] = array(
    '#title' => t('Force Group ID'),
    '#description' => t('Force all content into group.'),
    '#type' => 'textfield',
    '#default_value' => variable_get('os_migrate_force_og', FALSE),
  );

  $form['dev_fieldset']['os_migrate_legacy_site'] = array(
    '#title' => t('Legacy Site'),
    '#description' => t('(optional) Full URL to site from which content is being migrated.  Nodes that yield errors will attempt to link back to the original content if this setting is provided.'),
    '#type' => 'textfield',
    '#default_value' => variable_get('os_migrate_legacy_site', FALSE),
  );

  $form['dev_fieldset']['register_dynamic'] = array(
    '#type' => 'markup',
    '#markup' => '<strong>Dynamic Migration</strong><br />' . l('Re-run registration of dynamic migrations', 'admin/config/openscholar/os_migrate/register_dynamic'),
  );

  $form['dev_fieldset']['os_migrate_use_domain'] = array(
    '#title' => t('Treat source as domain'),
    '#default_value' => variable_get('os_migrate_use_domain'),
    '#type' => 'textfield',
    '#description' => t('OpenScholar migration includes a few edge case scenarios where a certain vsite gets special treatment.  To prevent this from interfering with the general case, the special treatment only happens when the legacy database has a certain base domain.  This setting overrides that domain.  Useful when you\'re trying to test migration of http://prod.com, but are doing so from http://dev.local.'),
  );

  $form['dev_fieldset']['os_migrate_restrict_vsites'] = array(
    '#title' => t('Restrict vsites'),
    '#default_value' => variable_get('os_migrate_restrict_vsites', ''),
    '#type' => 'textfield',
    '#description' => t('Limit migration to content from a specific vsite or comma separated list of vsites by id.  (ex: 1,3,10,100)'),
  );

  return system_settings_form($form);
}

/**
 * Validates os_migrate's admin form.
 * Tries to connect to supplied db.
 */
function os_migrate_admin_validate($form, &$form_state) {
  $os_migrate_database = $form_state['values']['os_migrate_database'];
  $os_migrate_files = $form_state['values']['os_migrate_files'];

  // try to connect to database to confirm it's set up correctly.
  if (strlen($os_migrate_database)) {
    try {
      Database::getConnection('default', $os_migrate_database);
      Database::closeConnection('default', $os_migrate_database);
      drupal_set_message('Connected to migration database.');
    } catch (DatabaseConnectionNotDefinedException $e){
      form_set_error('os_migrate_database', t('Unable to connect to specified database.  Check your settings.php configuration.'));
    }
  }

  // check that files dir exists
  if (strlen($os_migrate_files)) {
    if (($dh = @opendir($os_migrate_files)) != FALSE) {
      closedir($dh);
      drupal_set_message('Files directory exists.');
    } else {
      form_set_error('os_migrate_files', t('Unable to open files directory.  Please check that it exists and is writeable by the webserver.'));
    }
  }
}

/**
 * @function os_migrate_register_dynamic
 *
 * Reruns registration of dynamic migrations from hook_enable.  Useful for dev.
 */
function os_migrate_register_dynamic() {
  require_once drupal_get_path('module', 'os_migrate') . '/os_migrate.install';
  os_migrate_register_migrations();
  drupal_goto('admin/config/openscholar/os_migrate');
}