<?php

/**
 * @file Migrate special case tables that need to be normalized before migrate can handle them
 * New tables can then be used as sources for final migration
 */
class SpecialTablesOSMigration extends OSMigration {
  public function __construct() {
    parent::__construct(MigrateGroup::getInstance('specialtables'));
    $this->description = t('Special Tables');
    //$this->dependencies = array();

    $csv_columns = array(
      array('name', 'Name of this special case'),
      array('src', 'Source Table'),
      array('dst', 'Temporary Destination Table'),
    );

    //this mapping just loops over special tables coming from csv file (since that was the easiest source to use as a table list
    //actual copying happens during prepare row
    $this->source = new MigrateSourceCSV(drupal_get_path('module', 'os_migrate').'/specialtables.csv', $csv_columns);
    $this->destination = new MigrateDestinationTable('os_migrate_special_tables');
    $this->map = new MigrateSQLMap($this->machineName,
      array('name' => array('type'=>'varchar', 'length'=>255)),
      array('name' => array('type'=>'varchar', 'length'=>255))
      //MigrateDestinationTable::getKeySchema()
      );

    $this->addSimpleMappings(array('name', 'src', 'dst'));
  }

  public function prepareRow($row) {
    switch ($row->name) {

      case 'body_inline_objects':
        //get relevant node bodies
        $query = parent::d6_db_select('node_revisions', 'nr')
          ->fields('nr', array('vid', 'body'))
          ->condition(db_or()
            ->condition('body', '%<iframe%', 'LIKE')
            ->condition('body', '%<embed%', 'LIKE')
            ->condition('body', '%<object%', 'LIKE')
          );

        //select all the embedded content
        $embeds = array();
        foreach($query->execute() as $node) {
          $dom = filter_dom_load($node->body);

          foreach(array('iframe', 'object', 'embed') as $tag) {
            $items = $dom->getElementsByTagName($tag);
            for ($i=0,$len=$items->length ; $i<$len ; $i++) {
              $dom_item = $items->item($i);
              $embeds[] = array('html' => $dom->saveXML($dom_item), 'vid' => $node->vid);
            }
          }
        }

        //put content into our temp table.
        foreach ($embeds as $embed) {
          drupal_write_record('os_migrate_body_inline_objects', $embed);
        }
        break;

      default:
        break;
    }

    return TRUE;
  }

  //delete all the special temp tables during rollback
  //yes this breaks the ability to control how many items roll back at a time.
  public function preRollback(){
    $query = db_select('os_migrate_special_tables', 't')
      ->fields('t', array('dst'));
    foreach ($query->execute() as $table) {
      db_delete($table->dst)->execute();
    }
  }
}
