import importlib.util
import pathlib
import shutil
import tempfile
import unittest
import json

WORKER = pathlib.Path(__file__).resolve().parents[1] / "worker.py"
spec = importlib.util.spec_from_file_location("pet_evaluator_worker", WORKER)
worker = importlib.util.module_from_spec(spec)
spec.loader.exec_module(worker)


class WorkerTests(unittest.TestCase):
    def test_normalize_output(self):
        self.assertEqual(worker.normalize_output("5   \r\n\n"), "5")
        self.assertEqual(worker.normalize_output("A\nB\n"), "A\nB")
        self.assertNotEqual(worker.normalize_output("Hello"), worker.normalize_output("hello"))

    def test_load_cases_preserves_hidden_and_limits(self):
        cases = worker.load_cases(WORKER.parent / "testcases", 2)
        self.assertEqual(len(cases), 5)
        self.assertFalse(cases[0]["hidden"])
        self.assertTrue(cases[1]["hidden"])
        self.assertEqual(cases[0]["timeLimitMs"], 2000)
        self.assertEqual(cases[0]["memoryLimitMb"], 128)

    def test_all_seeded_exercises_have_cases(self):
        case_dir = WORKER.parent / "testcases"
        for exercise_id in range(1, 6):
            cases = worker.load_cases(case_dir, exercise_id)
            self.assertGreaterEqual(len(cases), 2)
            self.assertTrue(any(c["hidden"] for c in cases))
            self.assertTrue(any(not c["hidden"] for c in cases))

    def test_missing_cases_rejected(self):
        with self.assertRaises(ValueError):
            worker.load_cases(pathlib.Path("/definitely/missing"), 999)

    def test_invalid_hidden_field_rejected(self):
        with tempfile.TemporaryDirectory() as tmp:
            p = pathlib.Path(tmp) / "1.json"
            p.write_text(json.dumps([{"input":"", "expectedOutput":"x", "hidden":"yes", "timeLimitMs":2000, "memoryLimitMb":128}]), encoding="utf-8")
            with self.assertRaises(ValueError):
                worker.load_cases(pathlib.Path(tmp), 1)

    @unittest.skipUnless(shutil.which("g++"), "g++ not available")
    def test_compile_error_is_caught_locally(self):
        status, message = worker.run_local("int main( {", {"input":"", "timeLimitMs":1000, "memoryLimitMb":128})
        self.assertEqual(status, "compile_error")
        self.assertTrue(message)

    @unittest.skipUnless(shutil.which("g++"), "g++ not available")
    def test_infinite_loop_hits_timeout_locally(self):
        source = "int main(){while(true){} }"
        status, message = worker.run_local(source, {"input":"", "timeLimitMs":300, "memoryLimitMb":128})
        self.assertEqual(status, "time_limit_exceeded")
        self.assertIn("time limit", message.lower())

    @unittest.skipUnless(shutil.which("g++"), "g++ not available")
    def test_output_limit_is_enforced(self):
        source = '#include <iostream>\nint main(){for(int i=0;i<70000;i++) std::cout << "x";}'
        status, message = worker.run_local(source, {"input":"", "timeLimitMs":2000, "memoryLimitMb":128})
        self.assertEqual(status, "runtime_error")
        self.assertIn("64 KiB", message)

    @unittest.skipUnless(shutil.which("g++"), "g++ not available")
    @unittest.skipUnless(shutil.which("g++"), "g++ not available")
    def test_memory_limit_is_enforced(self):
        source = '#include <vector>\nint main(){ std::vector<char> data(200*1024*1024); data[0]=1; return 0; }'
        status, message = worker.run_local(source, {"input":"", "timeLimitMs":2000, "memoryLimitMb":32})
        self.assertEqual(status, "memory_limit_exceeded")
        self.assertIn("memory limit", message.lower())

    def test_correct_submission_passes_local_runner(self):
        source = '#include <iostream>\nint main(){int a,b; std::cin>>a>>b; std::cout<<a+b;}'
        case = {"input":"7 5\n", "expectedOutput":"12\n", "timeLimitMs":2000, "memoryLimitMb":128}
        status, output = worker.run_local(source, case)
        self.assertEqual(status, "ok")
        self.assertEqual(worker.normalize_output(output), "12")


if __name__ == "__main__":
    unittest.main()
