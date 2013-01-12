<?php

class SpacesOverridesShieldsMigration extends SpacesOverridesTableMigration {

  public function __construct() {
    $this->source_object = 'variable';
    $this->dest_object = 'boxes';
    $this->dependencies = array('SpacesOverridesBoxes');
    parent::__construct();
  }

  public function query() {
    $query = parent::query();
    $query->condition('object_id', 'vsite_shields_shield');
 //   $query->fields('spaces_overrides', array('id', 'value', 'type', 'object_type'));
 //   $query->distinct(FALSE);
    return $query;
  }

  public function prepareRow(&$row) {
    if (($value = @unserialize($row->value)) === FALSE) {
      if (empty($row->value) || $row->value == 'NULL') {
        return; //unless we have to update object_ids.
      }
      $value = $row->value; //serialize them all when we're done.
    }

    //get the migrated shield box
    $result = db_select('spaces_overrides', 'so')
      ->condition('object_type', 'boxes')
      ->condition('type', 'og')
      ->condition('id', $row->id)
      ->condition('value', '%os_boxes_shield%', 'LIKE')
      ->fields('so', array('value'))
      ->execute();

    if ($result->rowCount()) {
      //use existing
      parent::saveMessage(t('Ignoring shield for @nid.  Migrated with box override.', array('@nid'=>$row->id)), MigrationBase::MESSAGE_NOTICE);
      return FALSE;

    } else {
      //create new box
      $shield = (object) array(
        'changed' => NULL,
        'delta' => 'shield',
        'title' => '',
        'description' => 'Shield',
        'options' => array(
          'additional_classes' => '',
        ),
        'plugin_key' => 'os_shields_box',
      );
    }


    if (strlen($value)) {
      $shield_file = end(explode('/', $value));
    } else {
      $shield_file = '';
    }

    $shield->options['shield'] = $shield_file;

    $row->object_type = 'boxes';
    $row->object_id = 'shield';
    $row->value = serialize($shield);
    return parent::prepareRow($row);
  }

}
