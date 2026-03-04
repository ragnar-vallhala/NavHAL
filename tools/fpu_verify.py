import numpy as np

def generate_matrix_test_data(size=20):
    # Generate two random matrices
    A = np.random.rand(size, size).astype(np.float32) * 10
    B = np.random.rand(size, size).astype(np.float32) * 10
    
    # Compute result
    C = np.dot(A, B)
    
    print(f"// Generated {size}x{size} Matrices for FPU test")
    
    def print_matrix(name, mat):
        print(f"float {name}[{size}][{size}] = {{")
        for row in mat:
            print(f"    {{{', '.join([f'{x:.6f}f' for x in row])}}},")
        print("};")
    
    print_matrix("matrix_a", A)
    print()
    print_matrix("matrix_b", B)
    print()
    print_matrix("expected_result", C)

if __name__ == "__main__":
    generate_matrix_test_data()
