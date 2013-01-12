<?php

/**
 * @file
 * Migrate plugin to populate the OG vocabulary field by the term
 * references.
 */

class OsOgVocabMigrate extends DynamicMigration {


  public function __construct(array $arguments) {
    $this->arguments = $arguments;
    parent::__construct(MigrateGroup::getInstance('taxonomy',array('node')));
    $this->bundle = $bundle = $arguments['bundle'];

    $source_bundle = AbstractNodeOSMigration::get_source_bundle($bundle);

    $this->description = t('Populate the OG vocabulary field from a remote Drupal 6 source.');

    // @todo add dependency on node migrations
    $this->dependencies = array('VocabularyOS');

    $query = OSMigration::d6_db_select('term_node', 'tn');
    $query->innerJoin('term_data', 'td', 'tn.tid = td.tid');
    $query->innerJoin('og_vocab', 'ogv', 'td.vid = ogv.vid');
    $query->innerJoin('vocabulary', 'v', 'td.vid = v.vid');
    $query->innerJoin('node', 'n', 'tn.nid = n.nid');
    $query->addExpression("GROUP_CONCAT(DISTINCT tn.tid SEPARATOR ',')",'tid');
    $query->addExpression("GROUP_CONCAT(DISTINCT CONCAT_WS('|',tn.tid,tn.weight_in_tid) SEPARATOR ',')",'tid_weight');

    $query
      ->fields('tn',array('nid'))
      ->fields('v', array('multiple', 'required', 'tags'))
      ->condition('n.type', $source_bundle)
      ->orderBy('n.nid');

    //Group By Node
    $query->groupBy('nid');

    $source_key = MigrateDestinationNode::getKeySchema();
    // Set the alias, so the query in MigrateSourceSQL::performRewind()
    // will not fail.
    $source_key['nid']['alias'] = 'n';

    $this->query = $query;
    $this->map = new MigrateSQLMap($this->machineName, $source_key, MigrateDestinationNode::getKeySchema());

    // Create a MigrateSource object, which manages retrieving the input data.
    $this->source = new MigrateSourceSQL($this->query, array(), null, array('map_joinable' => FALSE));
    $this->destination = new MigrateDestinationNode($bundle);

    $this->addFieldMapping('nid', 'nid');

    //Add field mapping for vocabulary relatinships
    $this->addFieldMapping(OG_VOCAB_FIELD, OG_VOCAB_FIELD)->description('Handled in prepareRow');

    //Migrate using the exising nodes in the system
    $this->systemOfRecord = Migration::DESTINATION;
  }


  public function prepareRow($row) {

    $this->addOgVocab($row);
    $row->{OG_VOCAB_FIELD} = explode(',', $row->tid);

    $nodeorder_weights = explode(',', $row->tid_weight);
    $row->nodeorder = array();
    foreach ($nodeorder_weights as $no_info){
      list($tid,$weight) = explode("|" ,$no_info ,2 );
      $row->nodeorder[$tid] = $weight;
    }

    parent::prepareRow($row);
  }

  /**
   * Helper function to add the OG-vocab entity.
   */
  private function addOgVocab($row) {
    $terms = explode(',', $row->tid);
    foreach ($terms as $tid){
      $term = taxonomy_term_load($tid);
      $bundle = $this->arguments['bundle'];

      if(!$term){
        parent::saveMessage(t('The specified term %tid could not be loaded for bundle %bundle.',
          array('%tid' => $tid, '%bundle' => $bundle)), MigrationBase::MESSAGE_WARNING);
        return;
      }

      if (!og_vocab_load_og_vocab($term->vid, 'node', $bundle)) {
        // Create an OG vocab, based on the vocabulary settings.

        $settings = array(
          'required' => $row->required,
          'widget_type' => $row->tags ? 'entityreference_autocomplete_tags' : 'options_select',
          'cardinality' => $row->multiple || $row->tags ? FIELD_CARDINALITY_UNLIMITED : 1,
        );
        $og_vocab = og_vocab_create_og_vocab($term->vid, 'node', $bundle, $settings);
        $og_vocab->save();
      }
    }
  }

  /**
   * Construct the machine name from the source file name.
   */
  protected function generateMachineName($class_name = NULL) {
    return drupal_strtolower('OsOgVocabMigrate' . ucfirst($this->arguments['bundle']));
  }

  /**
   * Add the OG-vocab field to the bundle.
   */
  public function preImport() {
    $bundle = $this->arguments['bundle'];

    // Add OG-vocab field to bundle.
    if (!field_info_instance('node', OG_VOCAB_FIELD, $bundle)) {
      og_create_field(OG_VOCAB_FIELD, 'node', $bundle);
      self::displayMessage(t('Added "OG vocabulary" field to @bundle content-type.', array('@bundle' => $bundle)), 'success');
    }
  }

}
