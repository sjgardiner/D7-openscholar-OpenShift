<?php

/**
 *
 * Migration class CommentOSMigration
 *
 * Dynamic migration class for comments.
 * Make sure to register it following node migration!
 */
class CommentsOSMigration extends DynamicMigration {

  public function __construct(array $arguments){
    $this->description = t('OS taxonomy term migration');
    //Save the arguments for use later
    $this->arguments = $arguments;

    //This is used to create the instance ID
    $this->machineName = $this->generateMachineName();

    //Put all instances of this into the group "taxonomy"
    parent::__construct(MigrateGroup::getInstance('comments'));

    //Get all terms for this vocab ordered by parent
    $query = Database::getConnection('default', variable_get('os_migrate_database', ''))
      ->select('comments', 'c')
      ->fields('c');
    $query->join('node', 'n', 'c.nid = n.nid');
    $query->condition('n.type', AbstractNodeOSMigration::get_source_bundle($this->arguments['bundle']));
    $query->fields('n', array('language'));

    //The primary key in the D6 database
    $key_schema = array(
      'cid' => array(
        'type' => 'int',
        'unsigned' => TRUE,
        'not null' => TRUE,
        'description' => 'Legacy comment ID'
      )
    );

    //Create the Drupal 6 Source instance
    $this->source = new MigrateSourceSQL($query, $key_schema, null, array(
      'map_joinable' => FALSE
    ));

    //Create the D7 Destination
    $this->destination = new MigrateDestinationComment('comment_node_' . $this->arguments['bundle']);

    //Save the mapping between the D6 and D7 schema's
    $this->map = new MigrateSQLMap($this->machineName, $key_schema, MigrateDestinationComment::getKeySchema());

    $this->addSimpleMappings(array('nid', 'uid', 'pid', 'subject', 'hostname', 'name', 'mail', 'thread', 'homepage', 'status', 'language', 'path'));
    $this->addFieldMapping('created', 'timestamp');
    $this->addFieldMapping('changed', 'timestamp');

    $this->addFieldMapping('comment_body', 'comment');
    $this->addFieldMapping('comment_body:language', 'language');
    $this->addFieldMapping('comment_body:format', 'format');
  }

  /**
   * Implements prepareRow
   *
   **/
  public function prepareRow($current_row){
    if (parent::prepareRow($current_row) === FALSE) {
      return false;
    }

    //update format id to string
    if ($current_row->format == 0) {
      $current_row->format = 'plain_text';
    } else {
      $current_row->format = 'filtered_html';
    }

    //get the path
    $result = Database::getConnection('default', variable_get('os_migrate_database', ''))
      ->select('url_alias', 'ua')
      ->condition('src', 'node/'.$current_row->nid)
      ->fields('ua', array('dst'))
      ->execute();
    $current_row->path = $result->fetchColumn();

    return TRUE; // FALSE to skip a row.
  }

  protected function generateMachineName($class_name = NULL) {
    return strtolower('commentsosmigration' . $this->arguments['bundle']);
  }
}
