<?php

/**
 * @file os_migrate_node_stub.inc
 *
 * When creating new migration classes, use this as a starting point.  It includes placeholders
 * for all the functions you will have to fill out.
 *
 * Remember to add your new migration class to os_migrate.info.  Do not add the stub file to
 * os_migrate.info
 */

class StubNodeOSMigration extends AbstractNodeOSMigration {
  public function __construct() {

    /**
     * AbstractNodeOSMigration's __construct() will set up most of the common node migration.
     */
    parent::__construct('stub'); //args are d7 bundle, d6 content type.

    $this->description = t('Stub class for making content type migration classes.');

    $this->dependencies[] = 'StubNodeOS'; // append dependencies.  these classes must be migrated before this class.




  }

  public function query($content_type) {
    $query = parent::query($content_type);

    /**
     * Modify the query.  Add joins and fields here.  content_type_${content_type} and its fields are already included.
     */

    return $query;
  }


  public function sourceFields($content_type) {
    $fields = parent::sourceFields($content_type);

    /**
     * Describe each of the source columns
     */

    return $fields;
  }

  public function prepareRow($current_row) {

    /**
     * This method is called while looping over the results of the query provided earlier.
     *
     * This is your chance to munge data.  (See Abstract's filter formats).
     *
     * This is also your chance to query additional tables that didn't make sense to join earlier
     * (for example, see gallery.  d6's gallery had references to image nodes.  d7's gallery uses
     * file fields for images.  While looping over d6 gallery nodes, prepareRow uses additional queries
     * to get those image nodes and extract everything needed by file fields.)
     */

    return parent::prepareRow($current_row); //false -> skip this row

  }
}