Feature:
  Testing the comment publishing for a blog entry.

  @api
  Scenario: Check that a user can create a new blog post
    Given I am logged in as a user with the "administrator" role
     When I visit "john/blog"
      And I click "First blog"
      And I publish a new blog entry
     Then I should see "Lorem ipsum john doe"
