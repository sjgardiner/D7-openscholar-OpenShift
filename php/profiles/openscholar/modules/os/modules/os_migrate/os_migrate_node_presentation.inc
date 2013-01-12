<?php

/**
 * @file os_migrate_node_presentation.inc
 */

class PresentationNodeOSMigration extends AbstractNodeOSMigration {
  public function __construct() {

    /**
     * AbstractNodeOSMigration's __construct() will set up most of the common node migration.
     */
    parent::__construct('presentation'); //args are d7 bundle, d6 content type.

    $this->description = t('Presentation migration class.');

    //location
    $this->addFieldMapping('field_presentation_location', 'field_presentation_location_value');
    $this->addFieldMapping('field_presentation_location:format')->defaultValue('plain_text');
    $this->addFieldMapping('field_presentation_location:language', 'language')->defaultValue(LANGUAGE_NONE);

    //date
    $this->addFieldMapping('field_presentation_date', 'field_presentation_date_value');
    $this->addFieldMapping('field_presentation_date:to', 'field_presentation_date_value');
    $this->addUnmigratedDestinations(parent::subfields('field_presentation_date', array('timezone', 'rrule', 'from')), t('DNM'));

    //file
    $this->addFieldMapping('field_presentation_file', 'field_presentation_file')->defaultValue(NULL);
    $this->addSimpleMappings(parent::subfields('field_presentation_file', array('language','description','display')));
   // $this->addFieldMapping('field_presentation_file:destination_file', 'field_presentation_file_filename');
     $this->addFieldMapping('field_presentation_file:source_dir')
       ->defaultValue(variable_get('os_migrate_files') . '/' . parent::d6_variable_get('file_directory_path', 'sites/default/files'));

     $this->addFieldMapping('field_presentation_file:destination_dir', 'field_presentation_file_filepath');
    $this->addFieldMapping('field_presentation_file:og_group_ref', 'group_nid');

    //good defaults
    $this->addUnmigratedDestinations(parent::subfields('field_presentation_file', array('file_class','file_replace','preserve_files')));


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

  public function prepareRow(&$row) {
    $file_directory_path = parent::d6_variable_get('file_directory_path', 'sites/default/files');
    //$files_query = parent::os_migrate_node_get_files_query($row->vid);
    $files_query = self::d6_db_select('content_field_presentation_file', 'ct')
      ->fields('ct', array('field_presentation_file_fid', 'vid', 'field_presentation_file_data', 'field_presentation_file_list'))
      ->condition('ct.vid', $row->vid, '=');
    $files_query->join('files', 'f', 'ct.field_presentation_file_fid = f.fid');
    $files_query->addField('f', 'filepath');
    $files_query->addField('f', 'timestamp');
 //   $files_query->orderBy('ct.fid', 'ASC');

    $fields = array(
      'description' => 'field_presentation_file_data',
      'display' => 'field_presentation_file_list',
      'filepath' => 'filepath',
      'filename' => 'filename',
    );
    $query = $files_query->execute();
    $files = array();
    while ($file_row = $query->fetchObject()) {
      $filepath = $file_row->filepath;
      $filepath = str_replace($file_directory_path, '', $filepath);
      $file = explode('/', trim($filepath, '/'));

      $file_row->filename = array_pop($file);
      $new_filepath = parent::_get_destination_path('node', $row->nid);
      $file_row->filepath = ($new_filepath) ? $new_filepath : 'public://' . implode('/', $file);
      $file_row->field_upload_replace = $filepath;// . '/' . $file_row->filename;
      $files[] = $file_row;
    }

    parent::os_migrate_prepare_files($files, $row, 'field_presentation_file', $fields);
    //field upload is set up wrong, but it's too late to fix the general function.
    if (isset($row->field_presentation_file)) {
      $source_dir = variable_get('os_migrate_files') . '/' . parent::d6_variable_get('file_directory_path', 'sites/default/files');
      foreach (array_keys($row->field_presentation_file) as $delta) {
        $row->field_presentation_file[$delta] = $files[$delta]->field_upload_replace;
      }
      $row->field_presentation_file_source_dir = $source_dir;
      $row->field_presentation_file_filepath = current($row->field_presentation_file_filepath);//no longer takes array so just give it the first file's path.  they're all the same anyway.
    }

    return parent::prepareRow($row); //false -> skip this row
  }

  public function complete(&$entity, $row) {
    foreach ($entity->field_presentation_file as $LANG => $files) {
      foreach ($files as $delta => $file) {
        if ($f = file_load($file['fid'])) {
          if (isset($row->field_presentation_file_description[$delta]) && $data = @unserialize($row->field_presentation_file_description[$delta])) {
            if (isset($data['description']) && strlen($data['description'])) {
              $f->filename = $data['description'];
              file_save($f);
            }
          }
        }
      }
    }
  }

}