# Contributing to Gracemont Industrial Control Framework

Thank you for your interest in contributing to the Gracemont Industrial Control Framework! This document provides guidelines and information for contributors.

## Code of Conduct

Please be respectful and constructive in all interactions. We are committed to providing a welcoming and inclusive environment for everyone.

## How to Contribute

### Reporting Bugs

1. Check if the bug has already been reported in [Issues](https://github.com/Zixiao-System/gracemont/issues)
2. If not, create a new issue with:
   - Clear, descriptive title
   - Steps to reproduce
   - Expected vs actual behavior
   - System information (OS, compiler version, target architecture)
   - Relevant logs or error messages

### Suggesting Features

1. Open an issue with the `[Feature Request]` prefix
2. Describe the feature and its use case
3. Explain why it would benefit the project

### Submitting Code

1. Fork the repository
2. Create a feature branch (`git checkout -b feature/your-feature-name`)
3. Make your changes
4. Write or update tests as needed
5. Ensure all tests pass
6. Commit with clear, descriptive messages
7. Push to your fork
8. Open a Pull Request

## Development Guidelines

### Code Style

#### C Code (RTOS, Drivers, Protocols)

- Use 4 spaces for indentation (no tabs)
- Function names: `snake_case`
- Type names: `PascalCase` with `_t` suffix (e.g., `TaskHandle_t`)
- Macros and constants: `UPPER_SNAKE_CASE`
- Maximum line length: 100 characters
- Always use braces for control structures

```c
/* Good example */
void task_scheduler_init(SchedulerConfig_t *config)
{
    if (config == NULL) {
        return;
    }

    for (int i = 0; i < MAX_TASKS; i++) {
        task_list[i].state = TASK_STATE_INACTIVE;
    }
}
```

#### Header Files

- Use include guards with format: `_MODULE_NAME_H_`
- Group includes: system headers, project headers, local headers
- Document public APIs with comments

```c
#ifndef _SCHEDULER_H_
#define _SCHEDULER_H_

#include <stdint.h>
#include "rtos_types.h"

/**
 * @brief Initialize the task scheduler
 * @param config Pointer to scheduler configuration
 * @return 0 on success, negative error code on failure
 */
int scheduler_init(SchedulerConfig_t *config);

#endif /* _SCHEDULER_H_ */
```

### Commit Messages

Follow the conventional commits format:

```
type(scope): brief description

Detailed explanation if needed.

Fixes #123
```

Types:
- `feat`: New feature
- `fix`: Bug fix
- `docs`: Documentation changes
- `style`: Code style changes (formatting, no logic change)
- `refactor`: Code refactoring
- `test`: Adding or updating tests
- `chore`: Build process or auxiliary tool changes

### Testing

- Write unit tests for new functionality
- Ensure existing tests pass before submitting PR
- Test on target hardware when possible
- Include edge cases and error conditions

### Documentation

- Update README.md if adding new features
- Add API documentation for public functions
- Include usage examples where appropriate
- Keep documentation in sync with code

## Project Structure Guidelines

### Adding New Modules

When adding a new module:

1. Create appropriate directory structure
2. Add header files to `include/`
3. Add source files to module directory
4. Update Makefile
5. Add unit tests
6. Update documentation

### Safety-Critical Code

For code that may be used in safety-critical applications:

- Follow IEC 61508 guidelines where applicable
- Document all assumptions and constraints
- Avoid dynamic memory allocation in critical paths
- Use static analysis tools
- Provide detailed test coverage

## Review Process

1. All PRs require at least one review
2. CI checks must pass
3. Address all review comments
4. Squash commits if requested

## Getting Help

- Open a discussion for general questions
- Check existing issues and documentation
- Join our community chat (if available)

## License

By contributing, you agree that your contributions will be licensed under the GNU General Public License v3.0.

## Recognition

Contributors will be recognized in:
- Git commit history
- CONTRIBUTORS.md file (for significant contributions)
- Release notes

Thank you for contributing to open-source industrial automation!
