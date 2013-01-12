<?php

/**
 * @file
 * Implements TableCopyMigration for several direct table copies
 */

class IQSSPinserverRegisterTableMigration extends TableCopyMigration {

  public function __construct() {
    $this->required_modules = array('iqss_pinserver_register');
    $key_schema = array(
        'uid' => array(
            'type' => 'int',
            'unsigned' => TRUE,
            'not null' => TRUE,
        ),
    );

    $table = 'iqss_pinserver_register';
    parent::__construct($table, $key_schema);
  }
}


class CitationDistributeTableMigration extends TableCopyMigration {
  public function __construct() {
    $this->required_modules = array('citation_distribute');

    $key_schema = array(
      'nid' => array(
        'type' => 'int',
        'unsigned' => TRUE,
        'not null' => TRUE,
      ),
      'plugin' => array(
        'type' => 'varchar',
        'length' => 64,
        'not null' => TRUE,
      ),
    );

    $table = 'citation_distribute';

    parent::__construct($table, $key_schema);
  }
}

class VariableTableMigration extends TableCopyMigration {

  public function __construct() {
    $this->whitelist = array('mollom_private_key', 'mollom_public_key', 'os_mailchimp_api', 'os_mailchimp_list', 'olark_code',
      'apachesolr_host', 'apachesolr_path', 'apachesolr_port', 'iqss_rbuild_default_url', 'site_mail',
      'site_mission', 'site_name', 'site_offline_message', 'shorten_bitly_login', 'shorten_bitly_key',
      'iqss_gking::alternate_theme_nodes', 'iqss_gking::override_child_terms', 'iqss_gking::heading_disabled_terms',
      'embedfilter_host_whitelist', 'os_wysiwyg_fields_api_key',
    );

    $this->whitelist = array_merge($this->whitelist, parent::subfields('citation_distribute', array('dash_password', 'module_mode', 'service_googlescholar_auto_flag', 'service_repec_archive_code', 'service_repec_inst_name', 'service_repec_maintainer_email', 'service_repec_maintainer_name', 'service_repec_provider_homepage', 'service_repec_provider_institution', 'service_repec_title', 'service_repec_archive_path', 'service_repec_archive_url'), '_'));
    $this->whitelist = array_merge($this->whitelist, parent::subfields('iqss_gking', array('alternate_theme_nodes', 'heading_disabled_terms', 'override_child_terms'), '::'));
    $this->whitelist = array_merge($this->whitelist, parent::subfields('pinserver', array('app_name', 'gpg_dir', 'error_log', 'landing_path', 'pin_url'), '_'));
    // the following look like they should be migrated, but I'm not sure
    //pinserver*,

    //get biblio year vars
    $biblio_years = parent::d6_db_select('variable', 'v')
      ->condition('name', 'biblio_year_%', 'LIKE')
      ->fields('v', array('name'))
      ->execute()
      ->fetchCol();

    $this->whitelist = array_merge($this->whitelist, $biblio_years);

    $key_schema = array(
      'name' => array(
        'type' => 'varchar',
        'length' => 128,
        'not null' => TRUE,
      ),
    );

    $table = 'variable';
    parent::__construct($table, $key_schema);
  }

  public function query() {
    $query = parent::query();
    $query->condition('name', $this->whitelist, 'IN');
    return $query;
  }

  public function prepareRow($row) {
    //append biblio year to new array var instead of migrating
    if (strpos($row->name, 'biblio_year') === 0) {
      $years = variable_get('biblio_years_text', array(9999 => t('Submitted'), 9998 => t('In Press')));
      $new = end(explode('_', $row->name));
      $years[$new] = unserialize($row->value);
      variable_set('biblio_years_text', $years);
      return FALSE;
    }

    //update whitelist
    if ($row->name == 'embedfilter_host_whitelist') {
      $row->name = 'media_embed_whitelist';
      $row->value = serialize(explode("\r\n", unserialize($row->value)));
    }

    //rename variables
    $rename = array(
      'iqss_gking::alternate_theme_nodes'  => 'gkplus_minitheme_nodes',
      'iqss_gking::override_child_terms'   => 'gkplus_child_indent_terms',
      'iqss_gking::heading_disabled_terms' => 'gkplus_hidden_heading_terms',
      'os_wysiwyg_fields_api_key' => 'oembedembedly_api_key',
    );

    if (isset($rename[$row->name])) {
      $row->name = $rename[$row->name];
    }


  }

  //clear the menu cache after import.
  public function postImport() {
    parent::postImport();
    drupal_flush_all_caches();
  }
}

class UrlAliasTableMigration extends TableCopyMigration {

  public function __construct() {
    $key_schema = array(
      'pid' => array(
        'type' => 'int',
        'unsigned' => TRUE,
        'not null' => TRUE,
      ),
    );

    $table = 'url_alias';
    parent::__construct($table, $key_schema);

    //stash pids here so we don't overwrite any imported with nodes/users
    $this->used_pids = db_select('url_alias')->fields('url_alias', array('pid'))->execute()->fetchCol();
  }

  public function query() {
    $query = parent::query();

    $prefixes = array('nodeorder', 'iqss_gking_research_group', 'filefield_paths'); //taxonomies too?
    $or = db_or();
    foreach($prefixes as $prefix) {
      $or->condition('src', $prefix.'/%', 'LIKE');
    }

    $query->condition($or);

    return $query;
  }

  public function prepareRow(&$row) {
    $row->source = $row->src;
    unset($row->src);

    $row->alias = $row->dst;
    unset($row->dst);

    $row->pid = $this->getPID($row->pid);
  }

  //returns this pid if available.  if not, gets new one.  updates list
  function getPID($pid) {
    if (!in_array($pid, $this->used_pids)) {
      return $pid;
    }

    $new = 1 + max($this->used_pids);
    $this->used_pids[] = $new;
    return $new;
  }
}

class ContextualAnnotationsTableMigration extends TableCopyMigration {

  public function __construct() {
    $this->required_modules = array('contextual_annotation');
    $key_schema = array(
      'annotation_key' => array(
        'type' => 'varchar',
        'length' => 127,
        'not null' => TRUE,
      ),
      'nid' => array(
        'type' => 'int',
        'unsigned' => TRUE,
        'not null' => TRUE,
      ),
    );

    $table = 'contextual_annotation';
    parent::__construct($table, $key_schema);
  }

  public function prepareRow(&$row) {
    unset($row->annotation_type);
  }
}

class UsageLogTableMigration extends TableCopyMigration {
  public function __construct() {
    $this->required_modules = array('openscholar_log_usage');

    $key_schema = array(
      'host' => array(
        'type' => 'varchar',
        'length' => 255,
        'not null' => TRUE,
      ),
      'vsite' => array(
        'type' => 'varchar',
        'length' => 100,
        'not null' => TRUE,
      ),
      'age' => array(
        'type' => 'varchar',  //used to be datetime.
        'length' => 100,
        'not null' => TRUE,
      )
    );

    $table = 'openscholar_usage_log'; //why is the table named differently than the module?

    parent::__construct($table, $key_schema);
  }

  public function query() {
    //check if old table exists.
    if (parent::d6_db_table_exists('openscholar_usage_log')) {
      //return actual query
      return parent::query();
    } else {
      //return something null that won't give errors when called.
      return parent::d6_db_select('node', 'n')
        ->isNull('nid');

    }
  }

}

class PurlVsiteDomainTableMigration extends TableCopyMigration {

  public function __construct() {
    $key_schema = array(
      'value' => array(
        'type' => 'varchar',  //used to be datetime.
        'length' => 255,
        'not null' => TRUE,
      )
    );

    $table = 'purl';
    parent::__construct($table, $key_schema);

  }

  public function query() {
    $query = parent::query();
    $query->condition('provider', 'vsite_domain');
    return $query;
  }
}
