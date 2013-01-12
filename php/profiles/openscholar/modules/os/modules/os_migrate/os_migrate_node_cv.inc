<?php

/**
 * @file os_migrate_node_cv.inc
 *
 */

class CVNodeOSMigration extends AbstractNodeOSMigration {
  public function __construct() {

    /**
     * AbstractNodeOSMigration's __construct() will set up most of the common node migration.
     */
    parent::__construct('cv'); //args are d7 bundle, d6 content type.

    $this->description = t('CV node migration.');

    //$this->dependencies[] = 'StubNodeOS'; // append dependencies.  these classes must be migrated before this class.


    $this->addUnmigratedDestinations(
      parent::subfields('field_biocv_pdf_upload', array('file_class', 'language', 'file_replace', 'preserve_files')),
      t('DNM'));

    //cv pdf attachment
    //note: cv has two file fields.  cv_attach and cv_attachED.  in most cases they're identical.  when they aren't,
    //cv_attach is the one used in prod.  I'm keeping attach and ignoring attached.
    $this->addFieldMapping('field_biocv_pdf_upload', 'field_biocv_pdf_upload')
      ->defaultValue(NULL);
    $this->addFieldMapping('field_biocv_pdf_upload:destination_file');
    $this->addFieldMapping('field_biocv_pdf_upload:destination_dir');
    $this->addFieldMapping('field_biocv_pdf_upload:source_dir')
      ->defaultValue(variable_get('os_migrate_files') . '/' . parent::d6_variable_get('file_directory_path', 'sites/default/files'));
    //$this->addFieldMapping('field_biocv_pdf_upload:description', 'field_cv_attach_data'); //empty.  no sense in using it
    $this->addFieldMapping('field_biocv_pdf_upload:display', 'field_cv_attach_list');
    $this->addFieldMapping('field_biocv_pdf_upload:og_group_ref', 'group_nid');


    $this->addUnmigratedSources(array('field_cv_attach_fid', 'field_cv_attached_fid', 'field_cv_attached_list', 'field_cv_attached_data'), t('DNM'));

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

    $fields['field_biocv_pdf_upload'] = t('Uploaded CV file');

    return $fields;
  }

  public function prepareRow($current_row) {

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
    $file = parent::get_file_by_fid($current_row->field_cv_attach_fid);

    //there are two possible locations for cv files in the db.  check both of them.  verified by hand that their data doesn't conflict
    $alt_file = parent::get_file_by_fid($current_row->field_cv_attached_fid);
    $dir = variable_get('os_migrate_files', '') . '/';
    if ($file && parent::legacy_file_exists($dir . $file->filepath)) {
      if (list($path, $name) = $this->_migrate_filename($file->filepath)) {
        $current_row->{'field_biocv_pdf_upload:destination_file'}  = $name;
        $filepath = parent::_get_destination_path('node', $current_row->nid);
        $current_row->{'field_biocv_pdf_upload:destination_dir'} = ($filepath) ? $filepath : 'public://' . $path;
        $current_row->{'field_biocv_pdf_upload'} = $path . '/' . $name;
      }
    } elseif ($alt_file && parent::legacy_file_exists($dir . $alt_file->filepath)) {
    if (list($path, $name) = $this->_migrate_filename($alt_file->filepath)) {
        $current_row->{'field_biocv_pdf_upload:destination_file'}  = $name;
        $current_row->{'field_biocv_pdf_upload:destination_dir'} = 'public://' . $path;
        $current_row->{'field_biocv_pdf_upload'} = $path . '/' . $name;
      }
    }

    return parent::prepareRow($current_row); //false -> skip this row

  }
}