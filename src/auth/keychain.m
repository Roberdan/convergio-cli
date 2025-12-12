/**
 * CONVERGIO KEYCHAIN INTEGRATION
 *
 * Secure storage of API keys using macOS Keychain
 *
 * Copyright 2025 - Roberto D'Angelo & AI Team
 */

#import <Foundation/Foundation.h>
#import <Security/Security.h>
#include <string.h>
#include <stdlib.h>

// ============================================================================
// KEYCHAIN OPERATIONS
// ============================================================================

/**
 * Store a password in the Keychain
 * Returns 0 on success, -1 on failure
 */
int convergio_keychain_store(const char* service, const char* account, const char* password) {
    if (!service || !account || !password) {
        return -1;
    }

    @autoreleasepool {
        NSString* serviceStr = [NSString stringWithUTF8String:service];
        NSString* accountStr = [NSString stringWithUTF8String:account];
        NSData* passwordData = [NSData dataWithBytes:password length:strlen(password)];

        // First, try to delete any existing item
        NSDictionary* deleteQuery = @{
            (__bridge id)kSecClass: (__bridge id)kSecClassGenericPassword,
            (__bridge id)kSecAttrService: serviceStr,
            (__bridge id)kSecAttrAccount: accountStr
        };
        SecItemDelete((__bridge CFDictionaryRef)deleteQuery);

        // Now add the new item
        // Note: kSecAttrAccessible = WhenUnlocked for reasonable security
        // The item is only accessible when device is unlocked
        NSDictionary* addQuery = @{
            (__bridge id)kSecClass: (__bridge id)kSecClassGenericPassword,
            (__bridge id)kSecAttrService: serviceStr,
            (__bridge id)kSecAttrAccount: accountStr,
            (__bridge id)kSecValueData: passwordData,
            (__bridge id)kSecAttrAccessible: (__bridge id)kSecAttrAccessibleWhenUnlocked,
            // Sync to iCloud Keychain is disabled for API keys
            (__bridge id)kSecAttrSynchronizable: @NO
        };

        OSStatus status = SecItemAdd((__bridge CFDictionaryRef)addQuery, NULL);

        if (status == errSecSuccess) {
            return 0;
        } else if (status == errSecDuplicateItem) {
            // Item already exists, try to update it
            NSDictionary* updateQuery = @{
                (__bridge id)kSecClass: (__bridge id)kSecClassGenericPassword,
                (__bridge id)kSecAttrService: serviceStr,
                (__bridge id)kSecAttrAccount: accountStr
            };
            NSDictionary* updateAttrs = @{
                (__bridge id)kSecValueData: passwordData
            };

            status = SecItemUpdate((__bridge CFDictionaryRef)updateQuery,
                                   (__bridge CFDictionaryRef)updateAttrs);
            return (status == errSecSuccess) ? 0 : -1;
        }

        return -1;
    }
}

/**
 * Read a password from the Keychain
 * Returns allocated string on success (caller must free), NULL on failure
 */
char* convergio_keychain_read(const char* service, const char* account) {
    if (!service || !account) {
        return NULL;
    }

    @autoreleasepool {
        NSString* serviceStr = [NSString stringWithUTF8String:service];
        NSString* accountStr = [NSString stringWithUTF8String:account];

        NSDictionary* query = @{
            (__bridge id)kSecClass: (__bridge id)kSecClassGenericPassword,
            (__bridge id)kSecAttrService: serviceStr,
            (__bridge id)kSecAttrAccount: accountStr,
            (__bridge id)kSecAttrSynchronizable: @NO,
            (__bridge id)kSecReturnData: @YES,
            (__bridge id)kSecMatchLimit: (__bridge id)kSecMatchLimitOne
        };

        CFTypeRef result = NULL;
        OSStatus status = SecItemCopyMatching((__bridge CFDictionaryRef)query, &result);

        if (status == errSecSuccess && result != NULL) {
            NSData* passwordData = (__bridge_transfer NSData*)result;
            size_t length = [passwordData length];
            char* password = malloc(length + 1);
            if (password) {
                memcpy(password, [passwordData bytes], length);
                password[length] = '\0';
                return password;
            }
        }

        return NULL;
    }
}

/**
 * Delete a password from the Keychain
 * Returns 0 on success, -1 on failure
 */
int convergio_keychain_delete(const char* service, const char* account) {
    if (!service || !account) {
        return -1;
    }

    @autoreleasepool {
        NSString* serviceStr = [NSString stringWithUTF8String:service];
        NSString* accountStr = [NSString stringWithUTF8String:account];

        NSDictionary* query = @{
            (__bridge id)kSecClass: (__bridge id)kSecClassGenericPassword,
            (__bridge id)kSecAttrService: serviceStr,
            (__bridge id)kSecAttrAccount: accountStr,
            (__bridge id)kSecAttrSynchronizable: @NO
        };

        OSStatus status = SecItemDelete((__bridge CFDictionaryRef)query);

        return (status == errSecSuccess || status == errSecItemNotFound) ? 0 : -1;
    }
}

/**
 * Check if a password exists in the Keychain
 * Returns 1 if exists, 0 if not
 */
int convergio_keychain_exists(const char* service, const char* account) {
    if (!service || !account) {
        return 0;
    }

    @autoreleasepool {
        NSString* serviceStr = [NSString stringWithUTF8String:service];
        NSString* accountStr = [NSString stringWithUTF8String:account];

        NSDictionary* query = @{
            (__bridge id)kSecClass: (__bridge id)kSecClassGenericPassword,
            (__bridge id)kSecAttrService: serviceStr,
            (__bridge id)kSecAttrAccount: accountStr,
            (__bridge id)kSecAttrSynchronizable: @NO,
            (__bridge id)kSecMatchLimit: (__bridge id)kSecMatchLimitOne
        };

        OSStatus status = SecItemCopyMatching((__bridge CFDictionaryRef)query, NULL);

        return (status == errSecSuccess) ? 1 : 0;
    }
}
