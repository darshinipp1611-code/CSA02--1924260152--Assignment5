# CSA02 – Assignment 5 – Part A (Tasks A1, A2, A3)

Name: ____________   Register No.: ____________
GitHub: CSA02-<RegisterNumber>-Assignment5

| Folder | Task | Marks |
|--------|------|-------|
| A1/A1_Functional_Decomposition.md | Structure chart, prototypes, multiple return values, global-variable alternative | 7 |
| A2/A2_Recursion_Design.md | Digital root, fast power, call traces, recurrences, risks, alternatives | 5 |
| A3/A3_Pointer_Design.md | Pointer expressions + memory diagram, pointer-returning function, CLI design | 5 |

Part A is design work: all algorithms are written as pseudocode (no C source as a substitute).
Suggested commits: one per task (A1, A2, A3).

Common data assumptions (used in all tasks)
- Blood groups, index order: 0=A+, 1=A-, 2=B+, 3=B-, 4=AB+, 5=AB-, 6=O+, 7=O-
- Donor: id, name[50], group[4], age, phone[16], last donation date (Date: day, month, year)
- Stock: group_code, units  (fixed-size binary record)
- Eligibility rule (assumed, adjustable constants): age 18..65 AND at least 90 days since last donation
