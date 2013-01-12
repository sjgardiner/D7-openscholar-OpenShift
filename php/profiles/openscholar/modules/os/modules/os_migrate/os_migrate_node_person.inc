<?php

/**
 * @file os_migrate_node_person.inc

 */

class PersonNodeOSMigration extends AbstractNodeOSMigration {
  public function __construct() {

    /**
     * AbstractNodeOSMigration's __construct() will set up most of the common node migration.
     */
    parent::__construct('person'); //args are d7 bundle, d6 content type.

    $this->description = t('Person node migration class.');

    //$this->dependencies[] = 'StubNodeOS'; // append dependencies.  these classes must be migrated before this class.

    //text fields:
    $fields = array(
      'address' => 'address',
      'email' => 'email',
      'firstname' => 'first_name',
      'lastname' => 'last_name',
      'middlename' => 'middle_name_or_initial',
      'phone' => 'phone',
      'title' => 'professional_title',
    );

    foreach ($fields as $old => $new) {
      $this->addFieldMapping("field_${new}", "field_person_${old}_value");
      $this->addFieldMapping("field_${new}:format")->defaultValue('plain_text');
      $this->addFieldMapping("field_${new}:language", 'language');
    }

    //website
    $this->addFieldMapping('field_website', 'field_person_website_url');
    $this->addFieldMapping('field_website:title', 'field_person_website_title')->defaultValue(NULL);
    $this->addFieldMapping('field_website:attributes', 'field_person_website_attributes');


    //photo
    $this->addFieldMapping('field_person_photo', 'field_person_photo')
      ->defaultValue(NULL);
    $this->addSimpleMappings(parent::subfields('field_person_photo', array('destination_file', 'cropbox_x', 'cropbox_y', 'cropbox_height' ,'cropbox_width', 'alt', 'title')));
    $this->addFieldMapping('field_person_photo:source_dir', 'field_person_photo:source_dir')
      ->defaultValue(variable_get('os_migrate_files', NULL));
    $this->addUnmigratedDestinations(parent::subfields('field_person_photo', array('file_class', 'language', 'file_replace', 'preserve_files')));
    $this->addFieldMapping('field_person_photo:destination_dir', 'field_person_photo:destination_dir');
    $this->addFieldMapping('field_person_photo:imagefield_thumbs')->defaultValue(TRUE);


    //node abstract cleanup
    $this->removeFieldMapping('title');
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

    $crop_subfields = parent::subfields('field_person_photo', array('destination_file', 'cropbox_x', 'cropbox_y', 'cropbox_height', 'cropbox_width', 'alt', 'title'));
    $fields += array_combine($crop_subfields, $crop_subfields);
    $fields['filepath'] = t('Path to profile image');

    return $fields;
  }

  public function prepareRow(&$current_row) {

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

    /**
     * Dirty hack alert!
     *
     * gd upscaling has been crashing migrate.  until a better solution is found, we're going to
     * ignore files above 1mb for profile nodes.
     */
    if ($current_row->field_person_photo_fid && $data = unserialize($current_row->field_person_photo_data)) {

      $file = parent::get_file_by_fid($current_row->field_person_photo_fid);
      parent::os_migrate_prepare_files(array($file), $current_row, 'field_person_photo');
      $crop = $data['crop'];


      //if the file is too big, swap in its thumbnail and don't crop.
//       if ($file->filesize >= 1048576) {
//         foreach ($current_row->field_person_photo as $delta => $filepath) {
//           $thumbnail = $this->_get_thumbnail($filepath);
//           if ($thumbnail) {
//             $current_row->field_person_photo[$delta] = $thumbnail;
//             parent::saveMessage(t('Image too large to scale.  Replacing @file with @thumbnail',
//               array('@file' => $filepath, '@thumbnail' => $thumbnail)), MigrationBase::MESSAGE_NOTICE
//             );
//           } else {
//             $current_row->field_person_photo[$delta] = NULL;
//             parent::saveMessage(t('Image too large to scale.  No replacement found.  @file', array('@file' => $filepath)), MigrationBase::MESSAGE_WARNING);
//           }
//         }
//         $crop = array_diff_key($crop, array('x', 'y', 'width', 'height'));
//       }
//      $current_row->{'field_person_photo'} = $file->filepath;
      list ($dir, $filename) = parent::_migrate_filename($file->filepath);
      $current_row->{'field_person_photo:destination_file'} = $filename;
      $filepath = parent::_get_destination_path('node', $current_row->nid);
      $current_row->{'field_person_photo:destination_dir'} = ($filepath) ? $filepath : $dir;

      $current_row->{'field_person_photo:cropbox_x'} = $crop['x'];
      $current_row->{'field_person_photo:cropbox_y'} = $crop['y'];
      $current_row->{'field_person_photo:cropbox_width'} = $crop['width'];
      $current_row->{'field_person_photo:cropbox_height'} = $crop['height'];

      $current_row->{'field_person_photo:cropbox_alt'} = $data['alt'];
      $current_row->{'field_person_photo:cropbox_title'} = $data['title'];
    } else {
      $current_row->{'field_person_photo:source_dir'} = NULL;
    }

    if ((empty($current_row->field_person_website_title) || !$current_row->field_person_website_title) && ($current_row->field_person_website_url)){
      $current_row->field_person_website_title = t('Website');
    }

    //title should be in an array since there can be multiple
    if (isset($current_row->field_person_title_value)) {
      $current_row->field_person_title_value = array($current_row->field_person_title_value);
    }
    return parent::prepareRow($current_row); //false -> skip this row

  }

  //try to find a thumbnail to use instead of the full image.
  function _get_thumbnail($filepath) {
    $filename = end(explode('/', $filepath)); //file path includes sites/name/files.  need to iterate over these too.
    $thumbs_dir = array('imagefield_thumbs');
    $os_migrate_files = variable_get('os_migrate_files');

    foreach($thumbs_dir as $tdir) {
      $basepath = $os_migrate_files;
      $subpath = '';

      //can't generalize how much of os_migrate_files path to use as prefix, so test current, then add another chunk.
      foreach(explode('/', $filepath) as $tok) {
        $thumbnail = $subpath . '/' . $tdir . '/' . $filename;
        if (is_readable($basepath . '/' . $thumbnail)) {
          return $thumbnail;
        } else {
          $subpath .= '/' . $tok;
        }
      }
    }

    //last ditch effort.  save list of known bad images and their thumbnails.
    $images = array(
      'sites/scholar.iq.harvard.edu/files/harvardcentershanghai/files/110303_feature_2_ks_0.jpg' => 'sites/scholar.iq.harvard.edu/files/imagefield_thumbs/110303_feature_2_ks.jpg',
      'sites/scholar.iq.harvard.edu/files/harvardcentershanghai/files/111805_features_ks_189_0.jpg' => 'sites/scholar.iq.harvard.edu/files/imagefield_thumbs/111805_features_ks_189.jpg',
    );

    return (isset($images[$filepath])) ? $images[$filepath] : FALSE;
  }
}