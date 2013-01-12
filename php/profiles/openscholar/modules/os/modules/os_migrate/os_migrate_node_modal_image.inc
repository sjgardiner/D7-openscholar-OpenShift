<?php

/**
 * @file os_migrate_node_stub.inc
 */

class ModalImageNodeOSMigration extends OSMigration {
  public function __construct() {

    parent::__construct(); // nope!  AbstractNode assumes node->node
    $this->dependencies = array('UsersOS');


    $this->destination = new MigrateDestinationMedia();
    $this->description = t('Modal image node -> media migration class.');
    $this->source = new MigrateSourceSQL($this->query(), $this->sourceFields(), null, array('map_joinable' => FALSE));
    $this->map = new MigrateSQLMap($this->machineName,
      array( 'nid' => array(
        'type' => 'int',
        'not null' => TRUE,
        'unsigned' => TRUE,
        'description' => 'D6 modal_image node ID.',
        'alias' => 'n',
        )
    ),
      MigrateDestinationMedia::getKeySchema()
    );



    $this->addFieldMapping('source_dir')->defaultValue(
      variable_get('os_migrate_files') . '/' . parent::d6_variable_get('file_directory_path', 'sites/default/files'));

    $this->addSimpleMappings(array('uid', 'timestamp', 'path', 'filename', 'destination_dir'));
    $this->addUnmigratedDestinations(array('destination_file', 'file_replace', 'preserve_files'), t('DNM')); //good defaults
    $this->addUnmigratedSources(array('group_nid'), t('DNM')); //group nid is in prepare.  filename is in the full path

    $this->addFieldMapping('value', 'filepath');
    //$this->addFieldMapping('source_dir')->defaultValue(variable_get('os_migrate_files', FALSE)); //adequate or does 2.4 need full path?

  }

  public function query() {
    $query = parent::d6_db_select('node', 'n')
      ->fields('n', array('nid', 'uid'));
    $query->condition('n.type', 'modal_image');

    $query->join('content_type_modal_image', 'ct', 'n.vid = ct.vid');
    $query->fields('ct');

    $query->join('files', 'f', 'ct.field_modal_image_fid = f.fid');
    $query->fields('f');

//     $query->leftjoin('og_ancestry', 'og', 'n.nid = og.nid');
//     $query->fields('og', array('group_nid'));
//     $query->condition('og.group_nid', '0', '!=');

    /**
     * Modify the query.  Add joins and fields here.  content_type_${content_type} and its fields are already included.
     */

    return $query;
  }

  public function sourceFields() {
    return array(
      'uid' => t('Node owner\'s uid'),
      'filename' => t('Name of file'),
      'filepath' => t('Path to file'),
      'timestamp' => t('File creation timestamp'),
      'path' => t('URL alias'),
      'group_nid' => t('Group nid'),
      );
  }

  public function prepareRow($current_row) {
    //get og group
    $og = parent::d6_db_select('og_ancestry', 'og')
      ->condition('nid', $current_row->nid)
      ->fields('og', array('group_nid'))
      ->execute()
      ->fetchCol();

    if (!$og) {
      $og = $this->_infer_og($current_row->nid);
      if (empty($og)) {
        parent::saveMessage(t('Could not find group for modal image: @nid', array('@nid' => $current_row->nid)));
        return FALSE;
      }
    }

    $current_row->group_nid = $og;

    //get path from {url_alias}
    $query = parent::d6_db_select('url_alias', 'ua')
      ->fields('ua', array('dst'))
      ->condition('ua.src', 'node/'.$current_row->nid, '=');
    $results = $query->execute()->fetchAssoc();
    $current_row->path = $results['dst'];

    if (list($path, $name) = $this->_migrate_filename($current_row->filepath)) {
      $current_row->filename = $name;
      $filepath = parent::_get_destination_path('og', $og);
      $current_row->destination_dir = ($filepath) ? $filepath : 'public://' . $path;
      $current_row->filepath = $path . '/' . $name;
    }
  }


  //file is saved.  add it to usage table.
  public function complete($entity, stdClass $row) {
    file_usage_add($entity, 'file', 'file', $row->group_nid);
  }

  //get full list of migrated
  public function preRollback() {
    parent::preRollback();

    $query = db_query('SELECT destid1 FROM migrate_map_modalimagenodeos');
    $fids = $query->fetchCol('destid1');

    variable_set('os_migrate_modalimage_prerollback', serialize($fids));
  }

  //remove rolled back files from the usage table
  public function postRollback() {
    parent::postRollback();

    $query = db_query('SELECT destid1 FROM migrate_map_modalimagenodeos');
    $remaining_fids = $query->fetchCol('destid1');

    $pre_fids = variable_get('os_migrate_modalimage_prerollback', FALSE);
    variable_del('os_migrate_modalimage_prerollback');

    if ($pre_fids) {
      $pre_fids = unserialize($pre_fids);

      $deleted_fids = array_diff($pre_fids, $remaining_fids);
      foreach ($deleted_fids as $fid) {
        $file = (object) array('fid' => $fid);
        file_usage_delete($file, 'file', 'file');
      }
    }
  }

  //some modal images don't have a group.  this is incorrect.  but they're used by a box, which does have a group.  by our powers of inference....
  function _infer_og($nid) {
    $likenid = '%' . $nid . '%';
    //$regex = '.*nid:s:\d+:' . $nid . '.*';
    $og = parent::d6_db_select('spaces_overrides', 'so')
      ->condition('object_type', 'boxes')
     ->condition('value', '%os_boxes_modal_image%', 'LIKE')
     ->condition('value', $likenid, 'LIKE')
    //->condition('value', $regex, 'REGEXP')
      ->fields('so', array('id'))
      ->execute()
      ->fetchCol();
    return $og;
  }
}