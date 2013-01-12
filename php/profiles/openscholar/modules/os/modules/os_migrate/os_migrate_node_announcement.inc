<?php

class AnnouncementNodeOSMigration extends AbstractNodeOSMigration {
  public function __construct() {
    // Let AbstractNode handle the source, dest, and map.
    parent::__construct('news');
    $this->description = t('OS Announcement Nodes');


    //field_photo
    $this->addFieldMapping('field_photo', 'field_photo');
    $this->addFieldMapping('field_photo:og_group_ref', 'group_nid');
    $this->addSimpleMappings(parent::subfields('field_photo', array('language', 'alt', 'title', 'destination_file', 'destination_dir')));
      //->defaultValue(FALSE);
    $this->addFieldMapping('field_photo:source_dir', 'field_photo:source_dir')
      ->defaultValue(variable_get('os_migrate_files') . '/' . parent::d6_variable_get('file_directory_path', 'sites/default/files'));


    $this->addUnmigratedDestinations(array('field_photo:file_class', 'field_photo:file_replace',
        'field_photo:preserve_files', ), t('DNM'));

//     //field_upload
//     $this->addSimpleMappings(array('field_upload') + parent::subfields('field_upload', array('language', 'description', 'display',)));
//     $this->addFieldMapping('field_upload:destination_file', 'filepath');
//     $this->addFieldMapping('field_upload:source_dir')
//       ->defaultValue(variable_get('os_migrate_files', FALSE));
//     //good defaults
//     $this->addUnmigratedDestinations(parent::subfields('field_upload', array('file_class', 'language', 'destination_dir', 'file_replace', 'preserve_files',)));

    $this->addUnmigratedSources(parent::subfields('field_announcement_photo', array('fid', 'list', 'data'), '_'), t('DNM'));
  }

  public function query($content_type) {
    $query = parent::query($content_type);

    /**
     * Modify the query.  Add joins and fields here.
    */


    return $query;
  }

  public function prepareRow(&$row) {
    if ($row->field_announcement_photo_data && $data = unserialize($row->field_announcement_photo_data)) {
      $row->{'field_photo:alt'} = isset($data['alt']) ? $data['alt'] : '';
      $row->{'field_photo:title'} = isset($data['title']) ? $data['title'] : '';
    }

    $lang = 'field_photo:language';
    $row->$lang = $row->language;

    //get attached photo
    $result = parent::d6_db_select('files', 'f')
    	->condition('fid', $row->field_announcement_photo_fid, '=')
    	->fields('f')
    	->execute();

    $photo = $result->fetchAssoc();
    if (is_array($photo) && count($photo)) {
      $row->field_photo = $photo['filepath'];
      //$row->{'field_photo:destination_file'} = $photo['filepath'];

      if (list($path, $name) = $this->_migrate_filename($photo['filepath'])) {
        $filepath = parent::_get_destination_path('node', $row->nid);
        $row->{'field_photo:filename'} = $name;
        $row->{'field_photo:destination_dir'} = ($filepath) ? $filepath : 'public://' . $path;
        $row->field_photo = $path . '/' . $name;
      }
    }

//     //file attachments
//     $files_query = parent::os_migrate_node_get_files_query($row->vid);
//     $fields = array('description', 'list', 'filepath');
//     parent::os_migrate_prepare_files($files_query->execute(), $row, 'field_upload', $fields);

    return parent::prepareRow($row); //false -> skip this row
  }

}