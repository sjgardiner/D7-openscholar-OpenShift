<?php

/**
 * @file
 * Implements TableCopyMigration for each of the biblio tables we're copying.
 */

class BiblioContributorTypeTableMigration extends TableCopyMigration {
  public function __construct() {
    $key_schema = array(
        'auth_category' => array(
            'type' => 'int',
            'unsigned' => TRUE,
            'not null' => TRUE,
        ),
        'biblio_type' => array(
            'type' => 'int',
            'unsigned' => TRUE,
            'not null' => TRUE,
        ),
        'auth_type' => array(
            'type' => 'int',
            'unsigned' => TRUE,
            'not null' => TRUE,
        ),
    );

    $table = 'biblio_contributor_type';

    parent::__construct($table, $key_schema);
  }
}

class BiblioContributorDataTableMigration extends TableCopyMigration {
  public function __construct() {

    $key_schema = array(
      'cid' => array(
        'type' => 'int',
        'unsigned' => TRUE,
        'not null' => TRUE,
      ),
    );

    $table = 'biblio_contributor_data';

    parent::__construct($table, $key_schema);
  }

  public function query() {
    //$query = parent::query();
    $query = parent::d6_db_select($this->table, $this->alias)
      ->fields('b', array('cid','aka','drupal_uid','name','lastname','firstname','prefix','suffix','initials','affiliation','md5'));

    //invalidate the query to prevent migration if required modules are not present.
    if (isset($this->required_modules) && count($this->required_modules)) {
      $modules = array_map('module_exists', $this->required_modules);
      if (in_array(FALSE, $modules)) {
        //use node because we know it will exist.
        $query = parent::d6_db_select('node', 'n')
        ->condition('nid', 0)
        ->fields('n', array('nid'));
      }
    }

    return $query;
  }
}

class BiblioContributorTypeDataTableMigration extends TableCopyMigration {
  public function __construct() {

    $key_schema = array(
        'auth_type' => array(
            'type' => 'int',
            'unsigned' => TRUE,
            'not null' => TRUE,
        ),
    );

    $table = 'biblio_contributor_type_data';

    parent::__construct($table, $key_schema);
  }
}

class BiblioTypesTableMigration extends TableCopyMigration {
  public function __construct() {
    $key_schema = array(
      'tid' => array(
        'type' => 'int',
        'not null' => TRUE,
      ),
    );

    $table = 'biblio_types';

    parent::__construct($table, $key_schema);
  }
}

class BiblioFieldTypeTableMigration extends TableCopyMigration {
  public function __construct() {
    $key_schema = array(
      'tid' => array(
        'type' => 'int',
        'not null' => TRUE,
        'unsigned' => TRUE,
      ),
      'fid' => array(
        'type' => 'int',
        'not null' => TRUE,
        'unsigned' => TRUE,
      ),
    );

    $table = 'biblio_field_type';

    parent::__construct($table, $key_schema);
  }

  public function query() {
    $query = parent::query();
    //only migrate new types.  those provided by os_publications shouldn't re-migrate
    $query->condition('tid', 1000, '>=');
    return $query;
  }

  public function prepareRow(&$row) {
    $vtab = db_select('biblio_field_type', 'bft')
      ->condition('tid', 1)
      ->condition('fid', $row->fid)
      ->fields('bft', array('vtab'))
      ->execute()
      ->fetchField();

    $row->vtab = (is_int($vtab)) ? $vtab : 0;
  }
}

class BiblioFieldTypeDataTableMigration extends TableCopyMigration {
  public function __construct() {
    $key_schema = array(
      'ftdid' => array(
        'type' => 'int',
        'not null' => TRUE,
        'unsigned' => TRUE,
      ),
    );

    $table = 'biblio_field_type_data';

    parent::__construct($table, $key_schema);
  }
}

