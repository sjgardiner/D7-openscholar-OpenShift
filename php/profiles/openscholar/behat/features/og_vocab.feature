Feature:
  Testing the migrate of OG Vocabulary.

  @api @wip
  Scenario: Test OG Vocabulary migration
    Given I visit "john"
     When I click "Blog"
      And I should see "Antoine de Saint-Exupéry"
      And I should see "Douglas Noël Adams"
     Then I should see "Stephen William Hawking"
