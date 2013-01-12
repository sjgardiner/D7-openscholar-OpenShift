<?php
class SpacesOverridesVariableMigration extends SpacesOverridesTableMigration {

  public function __construct(){
    $this->source_object = $this->dest_object = 'variable';
    parent::__construct();
  }

  public function query(){
    $query = parent::query();
    //$query->condition('object_id', 'spaces_features');


    $blacklist = array(
      'vsite_design_disable_shield',
      'vsite_shields_disable_shield',
      'vsite_design_maintheme',
    );
    $query->condition('object_id', $blacklist, 'NOT IN');

    //black list for similar matches instead of exact.
    $like_blacklist = array(
      'vsite_comments_comment_type_%',
      'comment_default_%',
      'comment_anonymous_%',
      'comment_form_%',
      'comment_preview_%',
      'comment_subject_%',
      'comment_controls_%',
    );
    foreach ($like_blacklist as $not_like) {
      $query->condition('object_id', $not_like, 'NOT LIKE');
    }
    return $query;
  }

  public function prepareRow(&$row){
    //correct some known unserialized rows
    if ($row->value == '0') {
      $row->value = 'b:0;';
    }
    if ($row->value == '1') {
      $row->value = 'b:1;';
    }

    if (($value = @unserialize($row->value)) === FALSE) {
      if (empty($row->value) || $row->value == 'NULL') {
        return; //unless we have to update object_ids.
      }
      $value = $row->value; //serialize them all when we're done.
    }
    $object_id = $row->object_id;
    if ($this->_discontinued_variable($object_id)) {
      parent::saveMessage(t('Dropping variable: @var', array(
        '@var' => $object_id
      )), MigrationBase::MESSAGE_INFORMATIONAL);
      return FALSE;
    }

    //flavor variables get new names
    if (preg_match('/_flavor$/', $row->object_id)) {
      //$this->_update_flavors($row);
      list($object_id, $value) = $this->_update_flavors($object_id, $value, $row->id);
    }

    //scholar_publications variables:
    if (preg_match('/^scholar_publications/', $row->object_id)) {
      $object_id = $this->_update_variables_scholar_publications($row->object_id);
    }

    //comment by content type
    if (strpos($object_id, 'comment_') === 0 ) {
      list($object_id, $value) = $this->_update_variables_comment_content($row->id, $object_id, $value);
    }

    //Biblio Citation Style
    if ($object_id == 'biblio_style') {
      list($object_id, $value) = $this->_update_variable_biblio_style($row->id, $object_id, $value);
    }

    //FB Comments
//     if ($row->object_id == 'facebook_comments_nodetypes') {
//       $fb_preset = "Facebook Comments";
//       $row->object_id = "fb_social_{$fb_preset}_nodetypes";
//     }

    /*
     * pseudo-hook:  $this->_update_TYPE_ID($object_id, $value);
    * always returns array of paramters
    */
    $function = '_update_' . $row->object_type . '_' . $object_id;
    if (method_exists($this, $function)) {
      list($object_id, $value) = $this->$function($object_id, $value);
    }

    $row->object_id = $object_id;
    $row->value = serialize($value);
    return parent::prepareRow($row);
  }

  /*
   * @function _update_flavors
  *
  * Updates flavor variables and names.
  */
  public function _update_flavors($object_id, $value, $vsite){
    $replaced = 0;
    $object_id = preg_replace('/vsite_design_scholar/', 'os_appearance', $object_id, 1, $replaced);

    //Some themes start with vsite_design instead of vsite_design_scholar, handle those as well.
    if(!$replaced){
      $object_id = preg_replace('/vsite_design/', 'os_appearance', $object_id, 1);
    }

    //set the flavor manually for a few special cases
    if (parent::is_domain('http://scholar.harvard.edu')) {
      //agordon
      if ($vsite == 10057 && $object_id == 'os_appearance_eloquent_flavor') {
        $value = 'eloquent_sand_modfront';
      }
      //kshepsle
      if ($vsite == 1 && $object_id == 'os_appearance_eloquent_flavor') {
        $value = 'eloquent_green_modfront';
      }
      //rbates
      if ($vsite == 8 && $object_id == 'os_appearance_weft_flavor') {
        $value = 'w_african_nostrip';
      }
    }

    $value = preg_replace('/^scholar_/', '', $value);
    return array(
      $object_id,
      $value
    );
  }

  function _update_variable_biblio_style($sid, $object_id, $value) {

    if(module_exists('biblio_citeproc')){
      $object_id = "biblio_citeproc_style";

      $citeproc_map = array(
        "cse" => "cse.csl",
				"chicago" => "chicago-author-date.csl",// There are three to choose from??? chicago-author-date.csl  chicago-fullnote-bibliography.csl  chicago-note-bibliography.csl
				"apa" => "apa.csl",
				"ama" => "ama.csl",
				"mla" => "mla.csl",
				"ieee" => "ieee.csl",
				"vancouver" => "vancouver.csl",
      );
      //"classic" Not sure what 'classic' stye should migrate to

      $value = isset($citeproc_map[$value])? $citeproc_map[$value]:"cse.csl";

    }

    return array($object_id, $value);
  }

  //we're keeping the comment_$type variable, but using vsite_comment_vsite_comment_$type's value.
  function _update_variables_comment_content($id, $object_id, $value) {
    $content_type = str_replace('comment_', '', $object_id);
    $old_var = str_replace('comment_', 'vsite_comments_comment_type_', $object_id);
    $result = parent::d6_db_select('spaces_overrides', 'so')
      ->condition('id', $id)
      ->condition('object_type', 'variable')
      ->condition('object_id', $old_var)
      ->fields('so', array('value'))
      ->execute();

    if ($result->rowCount()) {
      $value = unserialize($result->fetchColumn());
      $value = ($value == 'drupal') ? 2 : 0; //unless we're using drupal, comment_sources will take over
    }

    $bundles = array_flip(AbstractNodeOSMigration::$source_bundle_map);
    if (isset($bundles[$content_type])) {
      $object_id = 'comment_' . $bundles[$content_type];
    }

    return array($object_id, $value);
  }

  public function _update_variables_scholar_publications($object_id){
    return preg_replace('/^scholar_/', 'os_', $object_id);
  }

  /**
   * pseudo hooks (for lack of a better term)
   *
   * To save a little space and keep from writing a million checks for object id, I'm using the type
   * and id for the name of a variable function.  _update_TYPE_ID.
   *
   * Takes object_id and value.  Returns array of the same to be set with list().
   */

  /**
   * Update the spaces_features variable by fixing the module names.
   */
  public function _update_variable_spaces_features($object_id, array $value){
    $spaces_features = array();
    foreach ( $value as $feature => $enabled ) {
      $spaces_features[$this->_update_module($feature)] = $enabled;
    }
    return array(
      $object_id,
      $spaces_features
    );
  }

  /**
   * spaces_preset_og variable
   */
  public function _update_variable_spaces_preset_og($object_id, $value){
    $presets = array(
      'scholar' => 'os_scholar',
      'project' => 'os_department'
    );

    return (isset($presets[$value])) ? array(
      $object_id,
      $presets[$value]
    ) : FALSE;
  }

  /**
   * variable: scholar_dvn_dvn_alias -> os_dataverse_dvn_alias
   */
  public function _update_variable_scholar_dvn_dvn_alias($object_id, $value){
    return array('os_dataverse_dvn_alias', $value);
  }

  /**
   * variable: scholar_biocv_use_pdf_as_cv -> os_biocv_use_pdf_as_cv
   */
  public function _update_variable_scholar_biocv_use_pdf_as_cv($object_id, $value){
    return array('os_biocv_use_pdf_as_cv', $value);
  }

  //vsite_ga_google_analytics_id => os_ga_google_analytics_id
  public function _update_variable_vsite_ga_google_analytics_id($object_id, $value){
    return array(
      'os_ga_google_analytics_id',
      $value
    );
  }

  //vsite_shields_shield_info => os_shields_shield_info
  public function _update_variable_vsite_shields_shield_info($object_id, $value){
    return array(
      'os_shields_shield_info',
      $value
    );
  }

  //no_year_text to new biblio variable
  public function _update_variable_biblio_no_year_text($object_id, $value){

    $a_year_text = variable_get('biblio_years_text', array(9999 => t('Submitted'), 9998 => t('In Press')));

    if(strlen($value)){
      $a_year_text[9999] = $value;
    }

    return array(
      'biblio_years_text',
      $a_year_text
    );
  }

  // vsite_shields_shield => os_shields_shield
  public function _update_variable_vsite_shields_shield($object_id, $value){
    return array(
      'os_shields_shield',
      $value
    );
  }

  //"scholar_book_toc_location" is now "os_booklets_toc_position".
  public function _update_variable_scholar_book_toc_location($object_id, $value) {
    return array('os_booklets_toc_position', $value);
  }

  //facebook nodetypes
  public function _update_variable_facebook_comments_nodetypes($object_id, $values) {
    return array('fb_social_Facebook Comments_nodetypes', $this->_update_comment_nodetypes($values));
  }

  //disqus nodetypes
  public function _update_variable_disqus_nodetypes($object_id, $values) {
    return array('disqus_nodetypes', $this->_update_comment_nodetypes($values));
  }

  // the *_nodetypes variables used by comment sources need updating.  instead of key => bool, they're key => key.
  public function _update_comment_nodetypes($values) {
    $values = array_filter($values);
    foreach($values as $key => $value) {
      $key = AbstractNodeOSMigration::get_source_bundle($key);
      $values[$key] = $key;
    }
    return $values;
  }

  // check if a variable is discontinued
  public function _discontinued_variable($object_id){
    static $discontinued;

    if (! isset($discontinued)) {
      $discontinued = array(
        'vsite_design_disable_shield',
        'vsite_shields_disable_shield',
        'vsite_design_maintheme', //Populated directly from og_theme
        'scholar_front_show_title' //No longer used
      );

      foreach ( node_type_get_names() as $bundle => $value ) {
        $node_type = AbstractNodeOSMigration::get_source_bundle($bundle);
        $discontinued = array_merge($discontinued, array(
          "comment_default_order_{$node_type}",
          "comment_default_mode_{$node_type}",
          "comment_controls_{$node_type}",
          "comment_anonymous_{$node_type}",
          "comment_default_per_page_{$node_type}",
          "comment_form_location_{$node_type}",
          "comment_preview_{$node_type}",
          "comment_subject_field_{$node_type}"
        ));
      }
    }

    return in_array($object_id, $discontinued);
  }


}



