<?php

/**
 *
 * Migration class TermOSMigration
 *
 * Migrates Taxonomy Terms dynamically, for each vocab that is created a
 * dynamic instance if TermOSMigration is created to migrate the terms for that vocab
 */
class TermOSMigration extends DynamicMigration {

  public function __construct(array $arguments){
    $this->description = t('OS taxonomy term migration');
    //Save the arguments for use later
    $this->arguments = $arguments;

    //This is used to create the instance ID
    $this->machineName = "TermOSMigrationvocabulary_" . $this->arguments['src_vid'];

    //Put all instances of this into the group "taxonomy"
    parent::__construct(MigrateGroup::getInstance('taxonomy',array('node')));

    //Get all terms for this vocab ordered by parent
    $query = Database::getConnection('default', variable_get('os_migrate_database', ''))->select('term_data', 'td')->fields('td', array(
      'tid',
      'name',
      'description',
      'weight'
    ))->orderBy('parent')->condition('td.vid', $arguments['src_vid'])->distinct();

    // Join to the hierarchy so we can sort on parent, but we'll pull the
    // actual parent values in separately in case there are multiples.
    $query->leftJoin('term_hierarchy', 'th', 'td.tid=th.tid');

    //The primary key in the D6 database
    $key_schema = array(
      'tid' => array(
        'type' => 'int',
        'unsigned' => TRUE,
        'not null' => TRUE,
        'description' => 'Source term ID'
      )
    );

    //Create the Drupal 6 Source instance
    $this->source = new MigrateSourceSQL($query, $key_schema, null, array(
      'map_joinable' => FALSE
    ));

    //Create the D7 Destination
    $this->destination = new MigrateDestinationTerm($this->arguments['dst_vocabulary']);

    //Save the mapping between the D6 and D7 schema's
    $this->map = new MigrateSQLMap($this->machineName, $key_schema, MigrateDestinationTerm::getKeySchema());

    // Mappings that are straight-forward 1-to-1
    $this->addSimpleMappings(array(
      'tid',
      'name',
      'description',
      'weight'
    ));
    $this->addFieldMapping('parent', 'parent')->sourceMigration($this->machineName);

    //The parent_name no longer exists in 7
    $this->addUnmigratedDestinations(array(
      'parent_name'
    ));

    //Path will be retrieved from the url_alias table if the path module is enabled
    if (module_exists('path')) {
      $this->addFieldMapping('path', 'path')->description('Handled in prepareRow');
    }

    //Disable pathauto, we will use exiting paths
    if (module_exists('pathauto')) {
      $this->addFieldMapping('pathauto')->description('By default, disable in favor of migrated paths')->defaultValue(0);
    }

    // Drupal 6 had no format for terms.
    $this->addFieldMapping('format');
  }

  /**
   * Implements prepareRow
   *
   **/
  public function prepareRow($current_row){
    if (parent::prepareRow($current_row) === FALSE) {
      return false;
    }

    //Add the (potentially multiple) parents
    $result = Database::getConnection('default', variable_get('os_migrate_database', ''))->select('term_hierarchy', 'th')->fields('th', array(
      'parent'
    ))->condition('tid', $current_row->tid)->execute();
    foreach ( $result as $parent_row ) {
      $current_row->parent[] = $parent_row->parent;
    }

    // Add the path to the source row, if relevant
    if (module_exists('path')) {

      $query = Database::getConnection('default', variable_get('os_migrate_database', ''))->select('url_alias', 'ua')->fields('ua', array(
        'dst'
      ))->condition('ua.src', 'taxonomy/term/' . $current_row->tid, '=');
      $results = $query->execute()->fetchAssoc();

      if ($results) {
        $current_row->path = $results['dst'];
      }
    }

    return TRUE; // FALSE to skip a row.
  }

  /**
   * Generate the machine name for this class
   */
  protected function generateMachineName() {
    return "TermOSMigrationvocabulary_" . $this->arguments['src_vid'];
  }
}
