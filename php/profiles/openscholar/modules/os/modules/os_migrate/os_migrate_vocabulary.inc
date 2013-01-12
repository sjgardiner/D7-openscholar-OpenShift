<?php
/**
 *
 * Migration class VocabularyOSMigration
 *
 * Migrates Taxonomy Vocabularies, for each vocab that migrated it will spawn an instance of TermOSMigration to migrate it's terms
 */
class VocabularyOSMigration extends TableCopyMigration {

  public function __construct() {

    $this->description = t('OS taxonomy vocab migration');

    //The D6 primary key for vocabularies
    $key_schema = array(
        'vid' => array(
            'type' => 'int',
            'unsigned' => TRUE,
            'not null' => TRUE,
        ),
    );

    $table = 'vocabulary';
    $dest_table = 'taxonomy_vocabulary';
    $group = MigrateGroup::getInstance('taxonomy',array('node'));

    //Create a one-to-one migrate from D6 vocabulary to D7 taxonomy_vocabulary
    parent::__construct($table, $key_schema, $group, $dest_table);

  }

  /**
   * Implements prepareRow
   *
   **/
  public function prepareRow($current_row) {

    //New Machine name field
    $current_row->machine_name = "vocabulary_".$current_row->vid;

    //No longer used fields
    unset($current_row->relations);
    unset($current_row->help);
    unset($current_row->multiple);
    unset($current_row->required);
    unset($current_row->tags);

    return TRUE;  // FALSE to skip a row.
  }

  /**
   * Implements postImport
   * @see Migration::postImport()
   *
   * After vocab import has completed create the term migrations for those vocabs that completed
   */
  public function postImport() {

    //register newly imported vocabs as term migrations.
//     $result = db_select('migrate_map_vocabularyos', 'm')
//       ->fields('m', array('destid1'))
//       ->execute();
    //Only register migrations for vocabs with terms
    $result = Database::getConnection('default', variable_get('os_migrate_database', ''))->select('term_data', 'td')->fields('td', array(
      'vid',
    ))->distinct()->execute();

    $vids = $result->fetchCol();

    foreach ($vids as $vid) {
      $arguments = array('dst_vocabulary' => "vocabulary_$vid", 'src_vid' => $vid);
      Migration::registerMigration('TermOSMigration', $this->_TermOSMachineName($vid), $arguments);
    }
  }

  //collect vocab os migrations to deregister later
  public function preRollback() {
    $result = db_select('migrate_map_vocabularyos', 'm')
      ->fields('m', array('destid1'))
      ->execute();

    //Only register migrations for vocabs with terms
//     $result = Database::getConnection('default', variable_get('os_migrate_database', ''))->select('term_data', 'td')->fields('td', array(
//       'vid',
//     ))->distinct()->execute();

    $vids = $result->fetchCol();

    variable_set('os_migrate_prerollback_vocabulary', serialize($vids));
  }

  public function postRollback() {
    $old_vids = variable_get('os_migrate_prerollback_vocabulary');
    $old_vids = ($old_vids) ? unserialize($old_vids) : array();
    $remaining_vids = db_select('migrate_map_vocabularyos', 'm')
      ->fields('m', array('destid1'))
      ->execute()
      ->fetchCol();

    $deleted_vids = array_diff($old_vids, $remaining_vids);

    //
    foreach ($deleted_vids as $vid) {
      Migration::deregisterMigration($this->_TermOSMachineName($vid));
    }

    variable_del('os_migrate_prerollback_vocabulary');
  }

  /**
   * The machine name to use for Term migrations
   *
   * @param int $vid
   * @return string
   */
  function _TermOSMachineName($vid) {
    return 'TermOSMigrationvocabulary_' . $vid;
  }


}


