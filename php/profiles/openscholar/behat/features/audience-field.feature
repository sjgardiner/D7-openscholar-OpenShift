Feature:
  Testing the appearance of OG audience field

  @api
  Scenario: Check that the OG audience field will be shown when editing content.
    Given I am logged in as a user with the "administrator" role
      And I visit "john/blog"
      And I click "First blog"
      And I click "Edit"
     Then I should not see "Groups audience"
