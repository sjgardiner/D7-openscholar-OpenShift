<?php

/**
 * @file
 * Migrate plugin to populate the OG vocabulary field by the term
 * references.
 */

class OsOgVocabMigratePrepare extends Migration {

  public function __construct() {
    parent::__construct(MigrateGroup::getInstance('taxonomy',array('node')));

    $this->description = t('Prepare the OG-vocab migration.');

    // TODO:
    $this->dependencies = array();

    $this->query = OSMigration::d6_db_select('og_vocab', 'ogv')
      ->fields('ogv');

    $key = array(
      'vid' => array(
        'type' => 'int',
        'unsigned' => TRUE,
        'description' => 'ID of destination term',
      ),
    );

    $this->map = new MigrateSQLMap($this->machineName, $key, $key);

    // Create a MigrateSource object, which manages retrieving the input data.
    $this->source = new MigrateSourceSQL($this->query, array(), null, array('map_joinable' => FALSE));
    $this->destination = new MigrateDestinationTable('og_vocab_relation');

    $this->addFieldMapping('gid', 'nid');
    $this->addFieldMapping('vid', 'vid');
    $this->addFieldMapping('group_type', NULL)->defaultValue('node');
  }
}
