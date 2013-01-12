<?php

/*
 * @class BookNodeOSMigration
 *
 * copies book node.  does NOT copy book structure, but depends on migrations that do.
 */
class BookNodeOSMigration extends AbstractNodeOSMigration {
  public function __construct() {
    $this->dependencies[] = 'BookTableOS';
    $this->dependencies[] = 'MenuLinksTableOS';

    // Let AbstractNode handle the source, dest, and map.
    parent::__construct('book');
    $this->description = t('OS Book Nodes');

  }

  function prepareRow($current_row) {
    return parent::prepareRow($current_row);
  }
}

//{book} needs copying
//books, class materials, and pages are all viable book pages.  anything else?

/**
 * @file
 * Make a copy of the book table.
 */

class BookTableOSMigration extends OSMigration {
  public function __construct() {
    parent::__construct();
    $this->dependencies = array();
    $this->description = t('Copy the book table.');

    $destination_key = array(
      'mlid' => array(
        'type' => 'int',
        'unsigned' => TRUE,
        'not null' => TRUE,
      ),
    );
    $fields = array('mlid', 'nid', 'bid');
    $query = parent::d6_db_select('book', 'b')->fields('b', $fields);
    $this->source = new MigrateSourceSQL($query, array_flip($fields), null, array('map_joinable' => FALSE));
    $this->destination = new MigrateDestinationTable('book', $destination_key);

    $this->map = new MigrateSQLMap($this->machineName,
      array('mlid' => array(
              'type' => 'int',
              'unsigned' => TRUE,
              'not null' => TRUE,
              )
            ),
      $destination_key
    );

    $this->addSimpleMappings($fields);
  }

}

/**
 * @class MenuLinksTableOSMigration
 *
 * copies book structure from menu_links
 **/

class MenuLinksTableOSMigration extends OSMigration {
  public function __construct() {
    parent::__construct();
    $this->dependencies = array();
    $this->description = t('Copy menu links for book table.');

    $destination_key = array(
      'mlid' => array(
        'type' => 'int',
        'unsigned' => TRUE,
        'not null' => TRUE,
      ),
    );

    $fields = array('menu_name', 'mlid', 'plid', 'link_path', 'router_path', 'link_title', 'options', 'module', 'hidden',
                    'external', 'has_children', 'expanded', 'weight', 'depth', 'customized', 'p1', 'p2', 'p3', 'p4', 'p5',
                    'p6', 'p7', 'p8', 'p9', 'updated');
    $query = parent::d6_db_select('menu_links', 'ml')
      ->fields('ml', $fields)
      ->condition('ml.module', 'book', '=');

    $this->source = new MigrateSourceSQL($query, array_flip($fields), null, array('map_joinable' => FALSE));
    $this->destination = new MigrateDestinationTable('menu_links', $destination_key);

    $this->map = new MigrateSQLMap($this->machineName,
      array('mlid' => array(
              'type' => 'int',
              'unsigned' => TRUE,
              'not null' => TRUE,
              )
            ),
      $destination_key
    );

    $this->addSimpleMappings($fields);
  }
}