<?php

/**
 * @file
 * Tasks for OpenScholar tasks.
 */

// Class to be extended by other os tasks classes.
class osTask extends og_task {
  public $site;
  public $account;

  function __construct($entity = NULL) {
    global $user;

    $this->site = $entity;
    $this->account = user_load($user->uid);

    if (empty($entity)) {
      $space = spaces_get_space();
      $this->site = $space->group;
    }
  }
}

// Create node.
class osTasksNodeCreate extends osTask {
  function __construct($entity = NULL) {
    parent::__construct($entity);
    $this->title = t("Create node");
    $this->description = t("Create a dummy content node.");
  }

  function execute() {
    $node = new stdClass();
    $node->type = 'page';
    $node->title = 'Dummy content';
    $node->language = LANGUAGE_NONE;
    $node->uid = $this->account->uid;
    node_save($node);

    og_group('node', $this->site, array(
      'entity_type' => 'node',
      'entity' => $node,
    ));
  }
}

// Create OG vocabulary.
class osTasksNodeVocab extends osTask {
  function __construct($entity = NULL) {
    parent::__construct($entity);
    $this->title = t("Create OG vocabulary");
    $this->description = t("Create a dummy OG vocabulary.");
  }

  function execute() {
    // Create a terms vocabulary and relate them to the groups.
    $vocabulary = new stdClass();
    $vocabulary->name = "Phones vocabulary";
    $vocabulary->machine_name = "phones_vocabulary";

    if (taxonomy_vocabulary_machine_name_load($vocabulary->machine_name)) {
      // Start iterating over machine names until we hit one that doesn't
      // exist yet.
      $i = 1;
      while (taxonomy_vocabulary_machine_name_load($vocabulary->machine_name)) {
        $vocabulary->machine_name = substr($vocabulary->machine_name, 0, 32 - strlen($i)) . $i;
        ++$i;
      }
    }

    // Creating the vocabulary.
    taxonomy_vocabulary_save($vocabulary);

    // Relate vocabulary to group.
    og_vocab_relation_save($vocabulary->vid, 'node', $this->site->nid);

    // Create OG vocabulary entity.
    $og_vocab = og_vocab_create_og_vocab($vocabulary->vid, 'node', 'page');
    $og_vocab->save();

    // Creating terms for the vocabulary.
    $phones = array(
      'iPhone 5',
      'Samsung Galaxy S III',
      'Sony Xperia Arc X12',
      'HTC One X'
    );

    foreach ($phones as $phone) {
      $term = new stdClass();
      $term->name = $phone;
      $term->vid = $vocabulary->vid;
      taxonomy_term_save($term);
    }
  }
}
