<?php

/**
 * @file os_migrate_flag.inc
 */


class FlagOSMigrate extends DynamicMigration {
  public function __construct(array $arguments) {
    $this->arguments = $arguments;
    $this->fid = $arguments['fid'];
    $this->legacy_fid = $arguments['legacy_fid'];

    parent::__construct(MigrateGroup::getInstance('flags'));

    $this->description = t('Flag migration.');
    $this->source = new MigrateSourceSQL($this->query(), array(), null, array('map_joinable' => FALSE));
    $this->destination = new MigrateDestinationFlagSimple($arguments['fid']);
    $this->map = new MigrateSQLMap(
      $this->machineName,
      array(
      	'fcid' => array(
          'type' => 'int',
          'not null' => TRUE,
          'unsigned' => TRUE,
          'description' => 'Flag/Content ID.',
        ),
      ),
      MigrateDestinationFlagSimple::getKeySchema()
    );

    $this->addSimpleMappings(array('content_type', 'content_id', 'uid', 'timestamp'));
    $this->addFieldMapping('fid')->defaultValue($this->fid);
    $this->addUnmigratedSources(array('fcid', 'fid (PK)', 'sid'), t('DNM'));
  }

  public function query() {
    $query = OSMigration::d6_db_select('flag_content', 'f')
      ->condition('fid', $this->legacy_fid)
      ->fields('f');

    /**
     * Modify the query.  Add joins and fields here.  content_type_${content_type} and its fields are already included.
     */

    return $query;
  }


  public function prepareRow(&$current_row) {

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

  /**
   * Construct the machine name from the source file name.
   */
  protected function generateMachineName($class_name = NULL) {
    return drupal_strtolower('FlagOSMigrate' . $this->arguments['fid']);
  }
}
