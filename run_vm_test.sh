#!/bin/bash

# This script will test the basic functionality of the miniSQL database.
# It will first remove any existing test database file, then it will
# start the miniSQL application, insert a record, select the record,
# and then exit.

echo "--- Starting VM Integration Test ---"

# Remove the old database file if it exists
rm -f test.db

# Run the test commands
# The `printf` command sends a sequence of commands to the miniSQL application.
# 1. "insert 1 user1 user1@example.com" - Inserts a new user record.
# 2. "select" - Retrieves all records from the database.
# 3. ".exit" - Exits the miniSQL application.
printf "insert 1 user1 user1@example.com\nselect\n.exit\n" | ./bin/miniSQL test.db

echo "--- Test Complete ---"
