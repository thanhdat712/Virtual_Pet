import importlib.util
import pathlib
import unittest

WORKER = pathlib.Path(__file__).resolve().parents[1] / "worker.py"
spec = importlib.util.spec_from_file_location("pet_evaluator_worker", WORKER)
worker = importlib.util.module_from_spec(spec)
spec.loader.exec_module(worker)

class WorkerTests(unittest.TestCase):
    def test_normalize_output(self):
        self.assertEqual(worker.normalize_output("5   \r\n\n"), "5")
        self.assertEqual(worker.normalize_output("A\nB\n"), "A\nB")
        self.assertNotEqual(worker.normalize_output("Hello"), worker.normalize_output("hello"))

    def test_load_existing_cases(self):
        cases = worker.load_cases(WORKER.parent / "testcases", 2)
        self.assertEqual(len(cases), 2)
        self.assertEqual(cases[0]["expectedOutput"], "12\n")

    def test_missing_cases_rejected(self):
        with self.assertRaises(ValueError):
            worker.load_cases(pathlib.Path("/definitely/missing"), 999)

if __name__ == "__main__":
    unittest.main()
